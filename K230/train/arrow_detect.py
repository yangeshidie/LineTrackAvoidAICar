import torch
import torch.nn as nn
import torch.optim as optim
from torchvision import transforms, datasets
from torch.utils.data import DataLoader
import time


# --- 1. 模型定义 (ArrowNet) ---
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


# --- 主执行函数 ---
if __name__ == "__main__":

    # --- 2. 超参数和设备设置 ---
    DEVICE = "cuda" if torch.cuda.is_available() else "cpu"
    print(f"Using {DEVICE} device")

    LEARNING_RATE = 0.001
    BATCH_SIZE = 32
    EPOCHS = 10  # 训练轮次，可以根据需要调整

    # --- 3. 数据加载 ---
    data_transform = {
        'train': transforms.Compose([
            transforms.Grayscale(num_output_channels=1),
            transforms.Resize((64, 64)),
            # 注意：在生成数据时已经做了旋转，这里可以不做或少做
            # transforms.RandomRotation(10),
            transforms.ToTensor(),
            transforms.Normalize(mean=[0.5], std=[0.5])
        ]),
        'val': transforms.Compose([
            transforms.Grayscale(num_output_channels=1),
            transforms.Resize((64, 64)),
            transforms.ToTensor(),
            transforms.Normalize(mean=[0.5], std=[0.5])
        ]),
    }

    data_dir = 'picture'
    train_dataset = datasets.ImageFolder(root=data_dir + '/train', transform=data_transform['train'])
    val_dataset = datasets.ImageFolder(root=data_dir + '/val', transform=data_transform['val'])

    train_loader = DataLoader(dataset=train_dataset, batch_size=BATCH_SIZE, shuffle=True)
    val_loader = DataLoader(dataset=val_dataset, batch_size=BATCH_SIZE, shuffle=False)

    print(f"训练集大小: {len(train_dataset)}, 验证集大小: {len(val_dataset)}")
    print(f"类别: {train_dataset.class_to_idx}")

    # --- 4. 初始化模型、损失函数和优化器 ---
    model = ArrowNet(num_classes=3).to(DEVICE)

    # 损失函数：交叉熵损失，适用于多分类问题
    criterion = nn.CrossEntropyLoss()

    # 优化器：Adam，一种常用的、效果很好的优化算法
    optimizer = optim.Adam(model.parameters(), lr=LEARNING_RATE)

    # --- 5. 训练循环 ---
    for epoch in range(EPOCHS):
        start_time = time.time()

        # --- 训练阶段 ---
        model.train()  # 设置为训练模式
        running_loss = 0.0
        for images, labels in train_loader:
            images, labels = images.to(DEVICE), labels.to(DEVICE)

            # 1. 清零梯度
            optimizer.zero_grad()

            # 2. 前向传播
            outputs = model(images)

            # 3. 计算损失
            loss = criterion(outputs, labels)

            # 4. 反向传播
            loss.backward()

            # 5. 更新权重
            optimizer.step()

            running_loss += loss.item() * images.size(0)

        epoch_loss = running_loss / len(train_dataset)

        # --- 验证阶段 ---
        model.eval()  # 设置为评估模式
        val_loss = 0.0
        corrects = 0
        with torch.no_grad():  # 在此模式下，不计算梯度，节省计算资源
            for images, labels in val_loader:
                images, labels = images.to(DEVICE), labels.to(DEVICE)

                outputs = model(images)
                loss = criterion(outputs, labels)

                val_loss += loss.item() * images.size(0)

                _, preds = torch.max(outputs, 1)
                corrects += torch.sum(preds == labels.data)

        epoch_val_loss = val_loss / len(val_dataset)
        epoch_acc = corrects.double() / len(val_dataset)

        end_time = time.time()

        print(f"Epoch {epoch + 1}/{EPOCHS} | "
              f"Time: {end_time - start_time:.2f}s | "
              f"Train Loss: {epoch_loss:.4f} | "
              f"Val Loss: {epoch_val_loss:.4f} | "
              f"Val Acc: {epoch_acc:.4f}")

    print("\nTraining finished!")

    # 可以选择保存训练好的模型权重
    torch.save(model.state_dict(), 'arrownet_model.pth')
    print("Model saved to arrownet_model.pth")