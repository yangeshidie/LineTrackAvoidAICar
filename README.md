# LineTrackAvoidAICar

## 项目简介 (Project Overview)
本项目是一个基于 **STM32F407** 和 **K230** 视觉模块的智能小车系统，集成了**循迹 (Line Tracking)**、**避障 (Obstacle Avoidance)** 和 **AI 视觉识别 (AI Visual Recognition)** 功能。

核心逻辑运行在 STM32 上，使用 **FreeRTOS** 实时操作系统进行任务调度。K230 模块负责视觉识别（如箭头方向识别），并通过 UART 与 STM32 通信。

## 硬件架构 (Hardware Architecture)
- **主控芯片**: STM32F407ZGT6
- **视觉模块**: Canaan K230 (运行 MicroPython)
- **传感器**:
  - **MPU6050**: 6轴姿态传感器 (用于精确转向和保持直线)
  - **超声波 (HC-SR04)**: 障碍物距离检测
  - **红外循迹模块**: 4路/5路红外传感器 (L1, L2, R1, R2)
- **执行器**:
  - **直流电机**: 差速驱动
  - **舵机 (SG90)**: 摄像头云台或转向机构
- **显示**: OLED 屏幕 (显示状态和调试信息)

## 软件架构 (Software Architecture)
项目基于 **STM32CubeMX** 生成，使用 **FreeRTOS** 进行多任务管理。

### 核心任务 (Core Tasks)
| 任务名称 | 优先级 | 功能描述 |
| :--- | :--- | :--- |
| `MotorConfig` | High | 电机控制核心循环，处理循迹算法 (PID) |
| `MVProcess` | AboveNormal4 | 视觉处理任务，解析 K230 发送的 UART 数据 |
| `PostureAcq` | AboveNormal2 | 读取 MPU6050 数据，计算欧拉角 (Yaw) |
| `ObstacleAvoidan`| Normal4 | 避障逻辑状态机 (倒车->绕行->回正) |
| `StateSwitch` | AboveNormal | 系统状态切换与管理 |
| `EncoderCap` | AboveNormal6 | 编码器数据采集 (用于测速) |
| `SG90Config` | Normal6 | 舵机控制 |
| `OLEDDisplay` | AboveNormal5 | OLED 屏幕刷新 |
| `DebugTask` | Realtime | 调试信息输出 |

## 目录结构 (Directory Structure)

```
LineTrackAvoidAICar/
├── Core/               # STM32 核心代码 (main.c, freertos.c, stm32f4xx_it.c)
├── Hardware/           # 硬件驱动程序
│   ├── Avoid.c         # 超声波避障逻辑
│   ├── LINE_TRACKER.c  # 红外循迹逻辑
│   ├── MOTOR.c         # 电机驱动与 PID 控制
│   ├── MPU6050.c       # 陀螺仪驱动
│   ├── OLED.c          # OLED 显示驱动
│   └── SG90.c          # 舵机驱动
├── K230/               # K230 视觉模块相关代码
│   ├── UART.py         # K230 运行的主脚本 (视觉识别 + 串口通信)
│   └── train/          # 模型训练相关文件
│       ├── arrow_detect.py # 数据采集/测试脚本
│       ├── arrownet.kmodel # 编译好的 KPU 模型文件
│       └── ...
├── Drivers/            # STM32 HAL 库
├── Middlewares/        # FreeRTOS 库
└── MDK-ARM/            # Keil 工程文件
```

## 功能说明 (Functionality)

### 1. 自动循迹 (Line Tracking)
- 使用红外传感器检测黑线/白线。
- 结合 PID 算法调整左右电机速度，保持小车在路径中心。

### 2. 智能避障 (Obstacle Avoidance)
- 当超声波检测到前方障碍物小于设定阈值时，触发避障任务。
- 避障流程：
    1. 停车并后退缓冲。
    2. 利用 MPU6050 陀螺仪辅助，精确左转 90 度。
    3. 直行绕过障碍物。
    4. 右转回到原路径方向。
    5. 寻找黑线并自动切回循迹模式。

### 3. AI 视觉识别 (AI Visual Recognition)
- K230 运行 `UART.py`，加载 `arrownet.kmodel` 模型。
- 识别视野中的箭头指示 (Left, Right, None)。
- 识别结果通过 UART (波特率 115200) 发送给 STM32。
- STM32 解析协议：
    - `AABBN...`: 颜色识别结果
    - `AAAABBB...`: 箭头识别结果

## 使用说明 (Usage)

### 1. STM32 工程 (STM32 Project)
> [!IMPORTANT]
> 为了保持仓库整洁，本仓库忽略了 `Drivers/` (HAL库), `Middlewares/` (FreeRTOS) 和 `MDK-ARM/` (Keil工程文件) 文件夹。
> **在编译之前，你需要重新生成这些文件。**

**恢复步骤 (Restoration Steps):**
1. 使用 **STM32CubeMX** 打开项目根目录下的 `XHcar.ioc` 文件。
2. 点击 **GENERATE CODE** 按钮。CubeMX 会自动下载并生成所需的 HAL 库、FreeRTOS 代码以及 Keil 工程文件。
3. 使用 **Keil uVision 5** 打开生成的 `MDK-ARM/XHcar.uvprojx`。
4. 编译并下载代码到 STM32F407 开发板。

### 2. K230 视觉模块
1. 将 `K230/UART.py` 和 `K230/train/arrownet.kmodel` (以及其他必要的模型文件) 复制到 K230 的文件系统中 (通常是 `/data/` 或 SD 卡)。
2. 确保 K230 的串口引脚 (TX/RX) 正确连接到 STM32 的对应串口 (USART2)。
3. 在 K230 上运行 `UART.py`：
   ```python
   python UART.py
   ```
   或者配置为开机自启。

### 3. 模型训练 (Model Training)
`K230/train` 文件夹包含了模型训练的资源。
- 如果需要更新识别模型（例如识别新的标志）：
    1. 采集数据集。
    2. 使用 PyTorch/TensorFlow 训练模型。
    3. 将模型导出为 ONNX 格式。
    4. 使用 Canaan 的工具链将 ONNX 编译为 `.kmodel` 格式。
    5. 替换 K230 中的 `arrownet.kmodel`。

## 贡献 (Contributing)
欢迎提交 Issue 和 Pull Request 改进代码。
