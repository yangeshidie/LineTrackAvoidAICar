# 导入所需库
import os
import argparse
import numpy as np
from PIL import Image  # 用于图像读取和处理
import onnxsim  # ONNX 模型简化工具
import onnx  # ONNX 模型处理工具
import nncase  # nncase 编译器 SDK
import shutil
import math


def parse_model_input_output(model_file, input_shape):
    # 加载ONNX模型
    onnx_model = onnx.load(model_file)

    # 获取模型中所有输入节点名称
    input_all = [node.name for node in onnx_model.graph.input]

    # 获取模型中已经被初始化的参数（如权重等），这些不属于输入数据
    input_initializer = [node.name for node in onnx_model.graph.initializer]

    # 真实输入 = 所有输入 - 初始化器
    input_names = list(set(input_all) - set(input_initializer))

    # 从图中提取真实输入张量
    input_tensors = [node for node in onnx_model.graph.input if node.name in input_names]

    # 提取输入张量的名称、数据类型、形状等信息
    inputs = []
    for _, e in enumerate(input_tensors):
        onnx_type = e.type.tensor_type
        input_dict = {}
        input_dict['name'] = e.name
        # 转换为NumPy数据类型
        # 这是最终的、正确的代码
        input_dict['dtype'] = onnx.helper.tensor_dtype_to_np_dtype(onnx_type.elem_type)
        # 如果某维为0，说明ONNX模型未固定shape，使用传入的input_shape代替
        input_dict['shape'] = [(i.dim_value if i.dim_value != 0 else d) for i, d in
                               zip(onnx_type.shape.dim, input_shape)]
        inputs.append(input_dict)

    return onnx_model, inputs


def onnx_simplify(model_file, dump_dir, input_shape):
    # 获取模型和输入形状信息
    onnx_model, inputs = parse_model_input_output(model_file, input_shape)

    # 自动推断缺失的shape信息
    onnx_model = onnx.shape_inference.infer_shapes(onnx_model)

    # 构造用于onnxsim的输入shape映射
    input_shapes = {input['name']: input['shape'] for input in inputs}

    # 简化模型
    onnx_model, check = onnxsim.simplify(onnx_model, input_shapes=input_shapes)
    assert check, "模型简化校验失败"

    # 保存简化后的模型
    model_file = os.path.join(dump_dir, 'simplified.onnx')
    onnx.save_model(onnx_model, model_file)
    return model_file


def read_model_file(model_file):
    with open(model_file, 'rb') as f:
        model_content = f.read()
    return model_content


# ---【修改点 1：适配灰度图的数据加载】---
# 原函数是为RGB三通道图像设计的，这里修改为处理单通道灰度图
def generate_data(shape, batch, calib_dir):
    # 获取数据集中的所有图片路径
    img_paths = [os.path.join(calib_dir, p) for p in os.listdir(calib_dir)]
    data = []

    for i in range(batch):
        assert i < len(img_paths), f"校准图片数量不足, 需要 {batch} 张, 但只找到 {len(img_paths)} 张"

        # 加载图片，并转换为 'L' (单通道灰度图)
        img_data = Image.open(img_paths[i]).convert('L')

        # 按模型输入尺寸进行缩放 (shape[3]是宽度, shape[2]是高度)
        img_data = img_data.resize((shape[3], shape[2]), Image.BILINEAR)

        # 转换为NumPy数组
        img_data = np.asarray(img_data, dtype=np.uint8)

        # 因为是单通道，需要手动增加一个通道维度，以符合 NCHW 格式
        # 从 (H, W) -> (1, H, W)
        img_data = np.expand_dims(img_data, axis=0)

        # 增加batch维度 (1, H, W) -> (1, 1, H, W)
        data.append([img_data[np.newaxis, ...]])

    return np.array(data)


def main():
    # 命令行参数定义
    parser = argparse.ArgumentParser(prog="nncase")
    parser.add_argument("--target", default="k230", type=str, help='编译目标，例如k230或cpu')
    parser.add_argument("--model", type=str, required=True, help='输入ONNX模型路径')
    parser.add_argument("--dataset_path", type=str, required=True, help='PTQ校准数据集路径')
    parser.add_argument("--input_width", type=int, required=True, help='模型输入宽度')
    parser.add_argument("--input_height", type=int, required=True, help='模型输入高度')
    parser.add_argument("--ptq_option", type=int, default=0, help='PTQ选项：0-5')

    args = parser.parse_args()

    # ---【修改点 2：适配ArrowNet模型的输入形状】---
    # 输入尺寸应与你的模型匹配。64已经是32的倍数，所以对齐操作不会改变它。
    input_width = int(math.ceil(args.input_width / 32.0)) * 32
    input_height = int(math.ceil(args.input_height / 32.0)) * 32
    # 将输入通道数从 3 修改为 1
    input_shape = [1, 1, input_height, input_width]  # NCHW格式

    # 创建临时目录保存中间模型
    dump_dir = 'tmp'
    if not os.path.exists(dump_dir):
        os.makedirs(dump_dir)

    # 简化模型
    print("Step 1: Simplifying ONNX model...")
    model_file = onnx_simplify(args.model, dump_dir, input_shape)
    print(f"ONNX model simplified and saved to {model_file}")

    # ---【修改点 3：适配ArrowNet模型的预处理参数】---
    # 编译选项设置
    compile_options = nncase.CompileOptions()
    compile_options.target = args.target  # 指定目标平台
    compile_options.preprocess = True  # 启用预处理
    compile_options.swapRB = False  # 灰度图此项无意义
    compile_options.input_shape = input_shape  # 设置输入形状
    compile_options.input_type = 'uint8'  # 输入图像数据类型

    # 这里的参数必须和你训练时 transforms.Normalize 的参数严格一致！
    # 你的设置为: transforms.Normalize(mean=[0.5], std=[0.5])
    # NNCase 的 mean/std 参数作用于 0-255 范围, 所以需要转换
    compile_options.input_range = [0, 255]  # 输入图片的像素值范围
    compile_options.mean = [0.5 * 255]  # (mean * 255) -> [127.5]
    compile_options.std = [0.5 * 255]  # (std * 255) -> [127.5]
    compile_options.input_layout = "NCHW"  # 输入数据格式

    # 初始化编译器
    compiler = nncase.Compiler(compile_options)

    # 导入ONNX模型为IR
    print("Step 2: Importing ONNX model to NNCase IR...")
    model_content = read_model_file(model_file)
    import_options = nncase.ImportOptions()
    compiler.import_onnx(model_content, import_options)
    print("Import complete.")

    # PTQ选项设置（后训练量化）
    print("Step 3: Setting up PTQ options...")
    ptq_options = nncase.PTQTensorOptions()
    ptq_options.samples_count = 20  # 校准样本数量，10-20张通常足够

    # 支持6种量化方案（根据精度与性能权衡选择）
    ptq_methods = [
        ('NoClip', 'uint8', 'uint8'), ('NoClip', 'uint8', 'int16'),
        ('NoClip', 'int16', 'uint8'), ('Kld', 'uint8', 'uint8'),
        ('Kld', 'uint8', 'int16'), ('Kld', 'int16', 'uint8')
    ]
    method, q_type, w_q_type = ptq_methods[args.ptq_option]
    ptq_options.calibrate_method = method
    ptq_options.quant_type = q_type
    ptq_options.w_quant_type = w_q_type
    print(f"Using PTQ option {args.ptq_option}: method={method}, quant_type={q_type}, w_quant_type={w_q_type}")

    # 设置PTQ校准数据
    print(f"Generating calibration data from {args.dataset_path}...")
    ptq_options.set_tensor_data(generate_data(input_shape, ptq_options.samples_count, args.dataset_path))

    # 应用PTQ
    compiler.use_ptq(ptq_options)
    print("PTQ setup complete.")

    # 编译模型
    print("Step 4: Compiling model...")
    compiler.compile()
    print("Compilation complete.")

    # 导出KModel文件
    base, ext = os.path.splitext(args.model)
    kmodel_name = base + ".kmodel"
    print(f"Step 5: Generating KModel file: {kmodel_name}...")
    with open(kmodel_name, 'wb') as f:
        f.write(compiler.gencode_tobytes())

    # 清理临时文件
    shutil.rmtree(dump_dir)
    print(f"\nSuccessfully generated {kmodel_name}!")


# Python程序主入口
if __name__ == '__main__':
    main()