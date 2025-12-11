import os
import random
from PIL import Image, ImageDraw, ImageOps
import numpy as np
from tqdm import tqdm

# --- 配置 ---
IMAGE_SIZE = 64
TRAIN_SAMPLES_PER_CLASS = 1000
VAL_SAMPLES_PER_CLASS = 200
BASE_DIR = 'dataset_v3_fixed'  # 改个名防止覆盖

CLASSES = ['left', 'right', 'none']
ROTATION_RANGE = (0, 180)  # 0到180度旋转


# --- 辅助函数 ---

def create_directories():
    if not os.path.exists(BASE_DIR):
        print(f"创建数据集目录: {BASE_DIR}")
    for split in ['train', 'val']:
        for class_name in CLASSES:
            os.makedirs(os.path.join(BASE_DIR, split, class_name), exist_ok=True)


def add_gaussian_noise(image):
    """最后的噪点处理，模拟摄像头噪点"""
    img_array = np.array(image, dtype=np.float32)
    mean = 0
    std_dev = random.uniform(2, 8)  # 稍微降低一点噪点，保证箭头清晰
    noise = np.random.normal(mean, std_dev, img_array.shape)
    noisy_array = img_array + noise
    noisy_array = np.clip(noisy_array, 0, 255)
    return Image.fromarray(noisy_array.astype('uint8'), 'L')


def generate_base_background():
    """
    生成背景：深灰色背景 + 黑色圆点 + 随机干扰线
    """
    # 背景色调暗一些 (50-100)，让白箭头突出来
    bg_color = random.randint(50, 100)
    image = Image.new('L', (IMAGE_SIZE, IMAGE_SIZE), color=bg_color)
    draw = ImageDraw.Draw(image)

    # 1. 绘制黑色圆点 (核心干扰项)
    num_dots = random.randint(3, 10)
    for _ in range(num_dots):
        r = random.randint(2, 4)
        x = random.randint(0, IMAGE_SIZE)
        y = random.randint(0, IMAGE_SIZE)
        # 黑色或深黑
        dot_color = random.randint(0, 30)
        draw.ellipse((x - r, y - r, x + r, y + r), fill=dot_color)

    # 2. 绘制一些随机线条 (模拟划痕或背景纹理)
    num_lines = random.randint(1, 3)
    for _ in range(num_lines):
        x1, y1 = random.randint(0, IMAGE_SIZE), random.randint(0, IMAGE_SIZE)
        x2, y2 = random.randint(0, IMAGE_SIZE), random.randint(0, IMAGE_SIZE)
        line_color = random.randint(40, 110)
        draw.line((x1, y1, x2, y2), fill=line_color, width=1)

    return image


def create_arrow_layer(direction):
    """
    在透明图层上绘制类似路牌的粗壮L型箭头。
    返回: RGBA Image
    """
    # 创建透明图层
    layer = Image.new('RGBA', (IMAGE_SIZE, IMAGE_SIZE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(layer)

    # 箭头颜色：高亮白/灰白 (200-255)
    arrow_color = (random.randint(220, 255), random.randint(220, 255), random.randint(220, 255), 255)

    # --- 尺寸参数 ---
    # 类似于 64x64 图片中的路牌比例
    cx, cy = IMAGE_SIZE // 2, IMAGE_SIZE // 2

    thickness = random.randint(7, 9)  # 线条粗细
    shaft_len = random.randint(16, 22)  # 竖杆长度
    arm_len = random.randint(14, 20)  # 横杆长度
    head_size = random.randint(10, 14)  # 三角形大小

    # 我们以 (cx, cy) 作为 L 的拐角点稍微往下一点的位置，尽量让整体重心居中
    # 拐角点坐标
    corner_x, corner_y = cx, cy + 5

    # 1. 绘制竖直粗线 (Shaft) - 总是向上
    # 从拐角向上画
    start_y = corner_y
    end_y = corner_y - shaft_len
    draw.line([(corner_x, start_y), (corner_x, end_y)], fill=arrow_color, width=thickness)

    # 2. 绘制水平粗线 (Arm) 和 箭头头部 (Head)
    if direction == 'left':
        # 向左弯
        arm_end_x = corner_x - arm_len

        # 画横线
        draw.line([(corner_x, corner_y), (arm_end_x, corner_y)], fill=arrow_color, width=thickness)

        # 画三角形箭头 (向左)
        # 尖端
        p1 = (arm_end_x - head_size + 2, corner_y)
        # 上翼
        p2 = (arm_end_x + 2, corner_y - head_size // 1.5)
        # 下翼
        p3 = (arm_end_x + 2, corner_y + head_size // 1.5)

        draw.polygon([p1, p2, p3], fill=arrow_color)

    elif direction == 'right':
        # 向右弯
        arm_end_x = corner_x + arm_len

        # 画横线
        draw.line([(corner_x, corner_y), (arm_end_x, corner_y)], fill=arrow_color, width=thickness)

        # 画三角形箭头 (向右)
        # 尖端
        p1 = (arm_end_x + head_size - 2, corner_y)
        # 上翼
        p2 = (arm_end_x - 2, corner_y - head_size // 1.5)
        # 下翼
        p3 = (arm_end_x - 2, corner_y + head_size // 1.5)

        draw.polygon([p1, p2, p3], fill=arrow_color)

    # 为了让拐角处连接自然，补一个圆或者矩形填充拐角空隙（因为draw.line默认是方头）
    # 简单做法：在拐角处画一个填充圆，直径等于thickness
    r = thickness / 2 - 0.5
    draw.ellipse((corner_x - r, corner_y - r, corner_x + r, corner_y + r), fill=arrow_color)

    return layer


def generate_sample(class_name):
    """
    生成单个样本：
    背景 -> 叠加旋转后的箭头 -> 噪声
    """
    # 1. 生成统一背景 (含黑点)
    base_image = generate_base_background()

    # 2. 如果不是 none 类，叠加箭头
    if class_name in ['left', 'right']:
        # 获取箭头图层
        arrow_layer = create_arrow_layer(class_name)

        # 随机旋转
        angle = random.uniform(ROTATION_RANGE[0], ROTATION_RANGE[1])

        # 旋转图层 (resample=Image.BICUBIC 保证边缘不锯齿)
        rotated_layer = arrow_layer.rotate(angle, resample=Image.BICUBIC, expand=False)

        # 叠加
        base_image.paste(rotated_layer, (0, 0), rotated_layer)

    # 3. 添加最后的高斯噪声
    final_image = add_gaussian_noise(base_image)

    return final_image


# --- 主程序 ---

def generate_dataset():
    create_directories()
    print(f"开始生成数据集...")
    print(f"配置: 尺寸{IMAGE_SIZE}x{IMAGE_SIZE}, 旋转{ROTATION_RANGE}, 箭头样式: 粗线L型")

    for split in ['train', 'val']:
        samples = TRAIN_SAMPLES_PER_CLASS if split == 'train' else VAL_SAMPLES_PER_CLASS

        for class_name in CLASSES:
            output_dir = os.path.join(BASE_DIR, split, class_name)
            print(f"正在生成 {split} - {class_name} ...")

            for i in tqdm(range(samples)):
                img = generate_sample(class_name)
                img.save(os.path.join(output_dir, f"{i:05d}.png"))

    print("\n搞定。请查看 dataset_v3_fixed 文件夹。")


if __name__ == '__main__':
    generate_dataset()