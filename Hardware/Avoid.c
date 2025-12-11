#include "Avoid.h"

/* 简单的微秒级延时系数 (根据主频调整) */
// STM32F4 (168MHz/84MHz) 粗略估算，如果不准可以微调
#define US_DELAY_MULT 50 
/* 传感器引脚读取宏 (保持不变) */
#define READ_L2  (HAL_GPIO_ReadPin(LINE_TRACKER_L2_GPIO_PORT, LINE_TRACKER_L2_GPIO_PIN) == GPIO_PIN_RESET)
#define READ_L1  (HAL_GPIO_ReadPin(LINE_TRACKER_L1_GPIO_PORT, LINE_TRACKER_L1_GPIO_PIN) == GPIO_PIN_RESET)
#define READ_R1  (HAL_GPIO_ReadPin(LINE_TRACKER_R1_GPIO_PORT, LINE_TRACKER_R1_GPIO_PIN) == GPIO_PIN_RESET)
#define READ_R2  (HAL_GPIO_ReadPin(LINE_TRACKER_R2_GPIO_PORT, LINE_TRACKER_R2_GPIO_PIN) == GPIO_PIN_RESET)
/**
 * @brief 简单的微秒延时 (阻塞式)
 */
static void Delay_us(uint32_t us)
{
    uint32_t i = us * US_DELAY_MULT;
    while(i--);
}

/**
 * @brief 初始化函数
 * 建议在 main.c 的 MX_GPIO_Init 中配置好引脚，这里主要做检查或复位
 */
void Obstacle_Init(void)
{
    // 确保 Trig 拉低
    HAL_GPIO_WritePin(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN, GPIO_PIN_RESET);
}

/**
 * @brief 读取超声波距离 (cm)
 * @return float 距离，如果超时返回 999.0
 */
float HCSR04_Read_Distance(void)
{
    uint32_t local_count = 0;
    uint32_t timeout = 0;

    // 1. 发送 Trig 脉冲 (至少 10us 高电平)
    HAL_GPIO_WritePin(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN, GPIO_PIN_SET);
    Delay_us(15);
    HAL_GPIO_WritePin(HCSR04_TRIG_PORT, HCSR04_TRIG_PIN, GPIO_PIN_RESET);

    // 2. 等待 Echo 变高 (超时处理)
    timeout = 100000;
    while(HAL_GPIO_ReadPin(HCSR04_ECHO_PORT, HCSR04_ECHO_PIN) == GPIO_PIN_RESET)
    {
        if(timeout-- == 0) return 999.0f; // 超时未响应
    }

    // 3. 测量 Echo 高电平时间
    // 注意：这里使用简单的循环计数，会被中断打断，精度一般但够用
    timeout = 100000; // 防止死循环
    while(HAL_GPIO_ReadPin(HCSR04_ECHO_PORT, HCSR04_ECHO_PIN) == GPIO_PIN_SET)
    {
        local_count++;
        Delay_us(1); // 延时1us
        if(timeout-- == 0) break;
    }

    // 4. 计算距离
    // 公式: 距离 = 时间(us) * 0.034cm/us / 2
    // 0.017 是理论值。如果发现测距不准（比如实际10cm测出20cm），请修改这个系数
    return (float)local_count * 0.017f; 
}

/**
 * @brief 执行避障流程 (阻塞式状态机)
 * 逻辑: 发现障碍 -> 左转90 -> 直行 -> 右转90 -> 直行(过障碍) -> 右转90(切回线) -> 找线 -> 左转回正
 */
void Run_Obstacle_Avoidance(void)
{	
    // 1. 挂起寻迹任务
    if(MotorConfigHandle != NULL) 
    {
        vTaskSuspend(MotorConfigHandle);
    }
	
    // 2. 倒车缓冲
    Car_Set_Speed(-20, -20);
    osDelay(200);
    Car_Set_Speed(0, 0);
    osDelay(200);
    
    // 【切记】不要在这里 Calibrate，除非你确定车绝对静止
	
    // 3. 第一阶段：左转绕出
    MPU6050_Turn_Angle(85.0f, 20); 
	
    // 4. 第二阶段：直行 (侧边)
    Car_Go_Straight_Gyro_Integration(20, 1000,100); // 删掉了多余的参数 100

    // 5. 第三阶段：右转平行
    MPU6050_Turn_Angle(-85.0f, 20);	

    // 6. 第四阶段：直行 (过障碍)
    Car_Go_Straight_Gyro_Integration(20, 1200,100);

    // 7. 第五阶段：右转切入
    MPU6050_Turn_Angle(-85.0f, 20);

    // 8. 第六阶段：找线 (修改了逻辑)
    // 不要用循环调 Gyro 函数，直接慢速开
    Car_Set_Speed(20, 20); 
    
    // 增加超时保护，防止永远找不到线死在这里
    uint32_t find_line_timeout = 1000; // 3秒超时
    uint32_t tick = 0;
    
    while(!(READ_L1 || READ_R1))
    {    
        osDelay(5);
        tick += 5;
        if(tick > find_line_timeout) break; // 超时强制退出
    }
    
    // 压线刹车
//    Car_Set_Speed(-20, -20);
//    osDelay(100);
    Car_Set_Speed(0, 0);
    osDelay(200); // 等稳一下

    // 9. 第七阶段：左转回正
    MPU6050_Turn_Angle(85.0f, 20);
    
    // 【关键修改】: 回正后，强制往前走一小步，防止立刻再次识别到障碍物
    // 这一步能有效解决“一直循环避障”的问题
    Car_Set_Speed(20, 20);
    osDelay(300); 
    
    // 恢复环境
    Line_Tracker_Init(); 
    
    // 10. 恢复任务
    if(MotorConfigHandle != NULL) 
    {
        vTaskResume(MotorConfigHandle);
    }
}