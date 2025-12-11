import os
import cv2
import numpy as np
import onnxruntime as ort
import nncase
from tqdm import tqdm  # 引入tqdm来显示进度条



def get_onnx_input(img_path, mean, std, model_input_size):
    image_fp32 = cv2.imread(img_path, cv2.IMREAD_GRAYSCALE)
    # 添加一个检查，如果图片读取失败则返回None
    if image_fp32 is None:
        return None
    image_fp32 = cv2.resize(image_fp32, (model_input_size[0], model_input_size[1]))
    image_fp32 = np.asarray(image_fp32, dtype=np.float32)
    image_fp32 /= 255.0
    image_fp32 -= mean[0]
    image_fp32 /= std[0]
    image_fp32 = np.expand_dims(image_fp32, axis=0)
    return image_fp32.copy()


def get_kmodel_input(img_path, model_input_size):
    image_uint8 = cv2.imread(img_path, cv2.IMREAD_GRAYSCALE)
    # 添加一个检查，如果图片读取失败则返回None
    if image_uint8 is None:
        return None
    image_uint8 = cv2.resize(image_uint8, (model_input_size[0], model_input_size[1]))
    image_uint8 = np.asarray(image_uint8, dtype=np.uint8)
    image_uint8 = np.expand_dims(image_uint8, axis=0)
    return image_uint8.copy()


def onnx_inference(ort_session, onnx_input_data):
    output_names = [output.name for output in ort_session.get_outputs()]
    model_input = ort_session.get_inputs()[0]
    model_input_name = model_input.name
    model_input_data = np.expand_dims(onnx_input_data, axis=0)
    onnx_results = ort_session.run(output_names, {model_input_name: model_input_data})
    return onnx_results


def kmodel_inference(sim, kmodel_input_data, model_input_size):
    input_shape = [1, 1, model_input_size[1], model_input_size[0]]
    dtype = sim.get_input_desc(0).dtype
    kmodel_input_data = np.expand_dims(kmodel_input_data, axis=0)
    kmodel_input = kmodel_input_data.astype(dtype).reshape(input_shape)
    sim.set_input_tensor(0, nncase.RuntimeTensor.from_numpy(kmodel_input))
    sim.run()
    kmodel_results = []
    for i in range(sim.outputs_size):
        kmodel_result = sim.get_output_tensor(i).to_numpy()
        kmodel_results.append(kmodel_result)
    return kmodel_results


def cosine_similarity(onnx_results, kmodel_results):
    onnx_i = np.reshape(onnx_results[0], (-1))
    kmodel_i = np.reshape(kmodel_results[0], (-1))
    cos = (onnx_i @ kmodel_i) / (np.linalg.norm(onnx_i, 2) * np.linalg.norm(kmodel_i, 2))
    return cos


def softmax(x):
    e_x = np.exp(x - np.max(x))
    return e_x / e_x.sum(axis=1)


if __name__ == '__main__':
    # --- 【修改点 1: 配置测试集路径和模型参数】 ---
    # 指定要遍历的测试集文件夹，这里我们使用验证集
    test_dir = "./dataset_v3_fixed/val"

    mean = [0.5]
    std = [0.5]
    model_input_size = [64, 64]
    onnx_model_path = "arrownet.onnx"
    kmodel_path = "arrownet.kmodel"

    # 你的类别名称，顺序必须和训练时 ImageFolder 识别的顺序一致
    class_names = ['left', 'none', 'right']

    # --- 【修改点 2: 初始化模型和统计变量】 ---
    print("Initializing models...")
    # 提前加载模型，避免在循环中重复加载
    ort_session = ort.InferenceSession(onnx_model_path)
    sim = nncase.Simulator()
    with open(kmodel_path, 'rb') as f:
        kmodel = f.read()
    sim.load_model(kmodel)

    # 初始化用于统计的列表和计数器
    cosine_scores = []
    onnx_correct_count = 0
    kmodel_correct_count = 0
    total_images = 0

    print(f"Starting validation on dataset: {test_dir}")
    # --- 【修改点 3: 遍历文件夹中的所有图片】 ---
    # 使用tqdm来创建一个漂亮的进度条
    with tqdm(total=sum([len(files) for r, d, files in os.walk(test_dir)])) as pbar:
        for class_index, class_name in enumerate(class_names):
            class_dir = os.path.join(test_dir, class_name)
            if not os.path.isdir(class_dir):
                continue

            for img_name in os.listdir(class_dir):
                img_path = os.path.join(class_dir, img_name)

                # 准备输入数据
                onnx_input = get_onnx_input(img_path, mean, std, model_input_size)
                kmodel_input = get_kmodel_input(img_path, model_input_size)

                if onnx_input is None or kmodel_input is None:
                    print(f"\nWarning: Skipping invalid image file {img_path}")
                    continue

                # 执行推理
                onnx_results = onnx_inference(ort_session, onnx_input)
                kmodel_results = kmodel_inference(sim, kmodel_input, model_input_size)

                # 累积余弦相似度
                cos_sim = cosine_similarity(onnx_results, kmodel_results)
                cosine_scores.append(cos_sim)

                # 累积准确率
                onnx_pred_index = np.argmax(softmax(onnx_results[0]))
                kmodel_pred_index = np.argmax(softmax(kmodel_results[0]))

                if onnx_pred_index == class_index:
                    onnx_correct_count += 1
                if kmodel_pred_index == class_index:
                    kmodel_correct_count += 1

                total_images += 1
                pbar.update(1)  # 更新进度条

    # --- 【修改点 4: 计算并打印最终的总结报告】 ---
    if total_images > 0:
        avg_cosine_similarity = np.mean(cosine_scores)
        onnx_accuracy = onnx_correct_count / total_images
        kmodel_accuracy = kmodel_correct_count / total_images

        print("\n\n--- Overall Validation Report ---")
        print(f"Total images tested: {total_images}")
        print(f"\nAverage Cosine Similarity: {avg_cosine_similarity:.6f}")
        print(f"\nONNX Model Accuracy:   {onnx_accuracy:.4f} ({onnx_correct_count}/{total_images})")
        print(f"Kmodel Model Accuracy: {kmodel_accuracy:.4f} ({kmodel_correct_count}/{total_images})")

        accuracy_drop = onnx_accuracy - kmodel_accuracy
        print(f"\nAccuracy Drop after quantization: {accuracy_drop:.4f}")
    else:
        print("\nError: No images were found in the specified test directory.")