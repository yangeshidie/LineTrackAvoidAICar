import torch
import torch.nn as nn


# --- 确保你的 ArrowNet 模型定义在这里 ---
# (和训练时使用的模型结构完全一样)
class ArrowNet(nn.Module):
    def __init__(self, num_classes=3):
        super(ArrowNet, self).__init__()

        def dw_block(in_ch, out_ch, stride=1):
            return nn.Sequential(
                nn.Conv2d(in_ch, in_ch, kernel_size=3, stride=stride, padding=1, groups=in_ch, bias=False),
                nn.BatchNorm2d(in_ch),
                nn.ReLU(inplace=True),
                nn.Conv2d(in_ch, out_ch, kernel_size=1, bias=False),
                nn.BatchNorm2d(out_ch),
                nn.ReLU(inplace=True),
            )

        self.features = nn.Sequential(
            nn.Conv2d(1, 16, 3, stride=2, padding=1),
            nn.BatchNorm2d(16),
            nn.ReLU(inplace=True),
            dw_block(16, 32, stride=2),
            dw_block(32, 64, stride=2),
            dw_block(64, 128, stride=2),
        )
        self.classifier = nn.Sequential(
            nn.AdaptiveAvgPool2d(1),
            nn.Flatten(),
            nn.Linear(128, num_classes)
        )

    def forward(self, x):
        x = self.features(x)
        x = self.classifier(x)
        return x


if __name__ == '__main__':
    # --- 导出配置 ---
    CHECKPOINT_PATH = 'arrownet_model.pth'  # 你训练好的模型权重文件
    ONNX_FILE_PATH = 'arrownet.onnx'  # 导出的ONNX文件名
    INPUT_SHAPE = (1, 1, 64, 64)  # 模型的输入形状 [N, C, H, W]

    # 1. 初始化模型并加载权重
    model = ArrowNet(num_classes=3)
    # 加载权重前，最好先将模型移动到CPU
    model.load_state_dict(torch.load(CHECKPOINT_PATH, map_location='cpu'))

    # 2. 设置为评估模式
    # !!! 这是非常重要的一步，它会固定BN层和关闭Dropout层
    model.eval()

    # 3. 创建一个符合模型输入的虚拟张量 (dummy input)
    dummy_input = torch.randn(INPUT_SHAPE, device='cpu')

    # 4. 执行导出
    torch.onnx.export(
        model,  # 要导出的模型
        dummy_input,  # 虚拟输入
        ONNX_FILE_PATH,  # 导出的文件名
        opset_version=11,  # ONNX算子集版本，11或12通常比较稳定
        input_names=['input'],  # 为输入节点命名
        output_names=['output'],  # 为输出节点命名
        dynamic_axes={'input': {0: 'batch_size'},  # 允许batch_size维度是动态的
                      'output': {0: 'batch_size'}}
    )

    print(f"模型已成功导出到: {ONNX_FILE_PATH}")
    print("下一步：请使用嘉楠K230的NNCase工具链将此ONNX文件转换为.kmodel")