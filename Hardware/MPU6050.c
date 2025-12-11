#include "MPU6050.h"
//#include "i2c.h"  // 必须包含，引用 hi2c1 句柄

/* 定义使用的I2C句柄，如果你用的是I2C2，请改为 &hi2c2 */
#define MPU_I2C_HANDLE  &hi2c2
#define I2C_TIMEOUT     100     // 超时时间 100ms

MPU6050_T g_tMPU6050; /* 全局变量，保存实时数据 */
float g_fZZeroError = 0.0f; // Z轴零偏误差
/*
*********************************************************************************************************
*	函 数 名: MPU6050_WriteByte
*	功能说明: 向 MPU-6050 寄存器写入一个数据 (HAL库版本)
*********************************************************************************************************
*/
static void MPU6050_WriteByte(uint8_t RegAddr, uint8_t Data)
{
    HAL_I2C_Mem_Write(MPU_I2C_HANDLE, MPU6050_ADDR, RegAddr, I2C_MEMADD_SIZE_8BIT, &Data, 1, I2C_TIMEOUT);
}

/*
*********************************************************************************************************
*	函 数 名: MPU6050_ReadByte
*	功能说明: 读取 MPU-6050 寄存器的一个数据 (HAL库版本)
*********************************************************************************************************
*/
static uint8_t MPU6050_ReadByte(uint8_t RegAddr)
{
    uint8_t Data;
    HAL_I2C_Mem_Read(MPU_I2C_HANDLE, MPU6050_ADDR, RegAddr, I2C_MEMADD_SIZE_8BIT, &Data, 1, I2C_TIMEOUT);
    return Data;
}

/*
*********************************************************************************************************
*	函 数 名: MPU6050_Init
*	功能说明: 初始化MPU-6050，配置寄存器
*********************************************************************************************************
*/
void MPU6050_Init(void)
{
    // 1. 复位设备，解除休眠
    MPU6050_WriteByte(PWR_MGMT_1, 0x00); 
     osDelay(50); // 稍微延时等待传感器稳定

    // 2. 设置采样率分频
    MPU6050_WriteByte(SMPLRT_DIV, 0x07);

    // 3. 设置低通滤波
    MPU6050_WriteByte(CONFIG, 0x06);

    // 4. 设置陀螺仪量程 (原例程为0xE8，建议改为常用配置 0x18 即2000deg/s，或者保持你的原值)
    // 这里保持你原例程的值，但通常 0xE8 会开启自检位，若数据异常建议改为 0x18
    MPU6050_WriteByte(GYRO_CONFIG, 0x18); 

    // 5. 设置加速度计量程 (0x01 对应 2g，通常配置为 0x00=2g 或 0x01=自检?)
    // 寄存器0x1C说明：Bit4:3为量程。00=2g, 01=4g, 10=8g, 11=16g。
    // 原例程 0x01 实际上只设置了最低位，可能没配准。通常建议 0x00(2g)
    MPU6050_WriteByte(ACCEL_CONFIG, 0x00); 
}

/*
*********************************************************************************************************
*	函 数 名: MPU6050_ReadData
*	功能说明: 连续读取 加速度、温度、角速度 数据
*********************************************************************************************************
*/
void MPU6050_ReadData(void)
{
    uint8_t ReadBuf[14];
    HAL_StatusTypeDef status;

    // 使用 HAL_I2C_Mem_Read 一次性读取14个字节
    // 从 ACCEL_XOUT_H (0x3B) 开始连读
    status = HAL_I2C_Mem_Read(MPU_I2C_HANDLE, MPU6050_ADDR, ACCEL_XOUT_H, I2C_MEMADD_SIZE_8BIT, ReadBuf, 14, I2C_TIMEOUT);
	
    if (status == HAL_OK)
    {
        // 合成数据：高8位 << 8 | 低8位
        g_tMPU6050.Accel_X = (int16_t)((ReadBuf[0] << 8) | ReadBuf[1]);
        g_tMPU6050.Accel_Y = (int16_t)((ReadBuf[2] << 8) | ReadBuf[3]);
        g_tMPU6050.Accel_Z = (int16_t)((ReadBuf[4] << 8) | ReadBuf[5]);

        g_tMPU6050.Temp    = (int16_t)((ReadBuf[6] << 8) | ReadBuf[7]);

        g_tMPU6050.Gyro_X  = (int16_t)((ReadBuf[8] << 8) | ReadBuf[9]);
        g_tMPU6050.Gyro_Y  = (int16_t)((ReadBuf[10] << 8) | ReadBuf[11]);
        g_tMPU6050.Gyro_Z  = (int16_t)((ReadBuf[12] << 8) | ReadBuf[13]);
    }
    else
    {
		
        // 这里可以添加错误处理，例如重置I2C或报错
    }
}

/*
*********************************************************************************************************
*	函 数 名: MPU6050_ReadID
*	功能说明: 读取器件ID，用于检测硬件连接是否正常
*   返 回 值: ID值，正常应该是 0x68
*********************************************************************************************************
*/
uint8_t MPU6050_ReadID(void)
{
    return MPU6050_ReadByte(WHO_AM_I);
}



/* ================= 【新增】核心功能函数 ================= */

/**
 * @brief Z轴零偏校准
 * 务必在电机未转动、车身静止时调用
 */
void MPU6050_Calibrate_Z(void)
{
    int32_t sum = 0;
    int sample_count = 200;

    // 读取多次求平均
    for(int i = 0; i < sample_count; i++)
    {
        MPU6050_ReadData();
        sum += g_tMPU6050.Gyro_Z;
        osDelay(2); // 间隔2ms
    }

    // 计算平均偏移量
    g_fZZeroError = (float)sum / (float)sample_count;
}

/**
 * @brief 闭环转向函数 (阻塞式)
 * @param target_angle: 目标角度 (正数左转，负数右转，单位：度)
 * @param speed: 转向时的电机PWM值
 */
/**
 * @brief 改进版闭环转向 (梯形积分 + 死区抑制)
 */
#include "stdio.h" // 用于 sprintf

/**
 * @brief 调试专用：带数据显示的闭环转向
 * @param target_angle: 目标角度
 * @param speed: 电机速度
 */
void MPU6050_Turn_Angle(float target_angle, int speed)
{	
	OLED_Init();
//	MPU6050_Init();
	
    double current_angle = 0.0f;
    float gyro_z_dps = 0.0f;
    uint32_t last_tick, now_tick;
    uint32_t last_print_tick = 0; // 用于控制OLED刷新频率
    float dt;
    
    // 显示目标角度
    OLED_Clear();
    OLED_DispString(1, 1, "Tgt:", Size8x16); 
    OLED_DispUNum(1, 5, (uint32_t)fabs(target_angle), Size8x16);

    // 1. 启动电机
    if (target_angle > 0) Car_Set_Speed(-speed, speed); 
    else Car_Set_Speed(speed, -speed);

    last_tick = HAL_GetTick();

    // 2. 循环积分
    while (fabs(current_angle) < fabs(target_angle))
    {
        // --- A. 数据读取与处理 ---
        MPU6050_ReadData();
        now_tick = HAL_GetTick();
        
        // 防止 dt = 0
        if(now_tick == last_tick) continue; 
        
        dt = (float)(now_tick - last_tick) / 1000.0f;
        last_tick = now_tick;

        // 计算角速度 (减去零偏 g_fZZeroError)
        // 【关键点】检查这里的 16.4 是否匹配你的量程配置
        gyro_z_dps = ((float)g_tMPU6050.Gyro_Z - g_fZZeroError) / 16.4f;

        // 积分
        current_angle += gyro_z_dps * dt;
        
        // --- B. 调试显示 (每200ms刷新一次，防止卡顿) ---
        if (now_tick - last_print_tick > 200)
        {
            last_print_tick = now_tick;
            
            // 第2行：当前角度 (Cur)
            OLED_DispString(2, 1, "Cur:", Size8x16);
            // 因为DispUNum只能显正数，判断符号
            if(current_angle < 0) OLED_DispString(2, 5, "-", Size8x16);
            else OLED_DispString(2, 5, "+", Size8x16);
            OLED_DispUNum(2, 6, (uint32_t)fabs(current_angle), Size8x16);
            
            // 第3行：实时角速度 (Spd)
            OLED_DispString(3, 1, "Gyr:", Size8x16);
            OLED_DispUNum(3, 5, (uint32_t)fabs(gyro_z_dps), Size8x16);
            OLED_DispSNum(4,1,(uint32_t)g_tMPU6050.Gyro_X,Size8x16);
            // 如果有串口，建议用 printf 输出，比 OLED 更准
            // printf("Ang: %.2f, Gyro: %.2f\r\n", current_angle, gyro_z_dps);
        }

        osDelay(2); // 极短延时，保证采样率
    }

    // 3. 停车并显示最终结果
	if (target_angle > 0) Car_Set_Speed(speed, -speed); 
    else Car_Set_Speed(-speed, speed);
	osDelay(200);
    Car_Set_Speed(0, 0);
    
    // 最终结果定格显示
    OLED_Clear();
    OLED_DispString(1, 1, "Done!", Size8x16);
    OLED_DispString(2, 1, "Ang:", Size8x16);
    if(current_angle < 0) OLED_DispString(2, 5, "-", Size8x16);
    OLED_DispUNum(2, 6, (uint32_t)fabs(current_angle), Size8x16);
    
    osDelay(2000); // 暂停2秒让你看清结果
}

/**
 * @brief 积分法-陀螺仪走直线 (无需DMP角度，仅需原始角速度)
 * @param Speed 基础速度
 * @param Time_ms 持续时间
 */
void Car_Go_Straight_Gyro_Integration(int16_t Speed, uint32_t Time_ms,uint32_t sharpbrake_time )
{	
//	MPU6050_Init();
    float current_angle = 0.0f; // 初始角度视为0

    int16_t turn_adjust = 0;
    
    // PID参数：Kp
    // 如果车左右摆动太厉害，减小这个值 (如 0.5)
    // 如果车修正不回来，增大这个值 (如 2.0)
    float Kp = 1.5f; 

    uint32_t tick = 0;
    uint32_t period = 10; // 采样周期 10ms = 0.01s

    while(tick < Time_ms)
    {
		MPU6050_ReadData();
        float gyro_dps = ((float)g_tMPU6050.Gyro_Z-g_fZZeroError) / 16.4f; // 这里的16.4需根据你初始化的量程修改
		
        // 2. 积分计算当前偏航角
        // 角度 = 角速度 * 时间(秒)
        // 假如角速度有静态漂移（静止时不为0），这里可能需要减去一个offset
        if(fabs(gyro_dps) > 1.0f) // 设置一个死区，过滤微小震动
        {
            current_angle += gyro_dps * (period / 1000.0f);
        }

        // 3. 计算修正量 (目标是保持角度为0)
        // 误差 = 0 - current_angle
        turn_adjust = (int16_t)((0.0f - current_angle) * Kp);

        // 4. 应用速度
        // 如果发现修正反了（越跑越歪），把下面的 - 和 + 互换
        Car_Set_Speed(Speed - turn_adjust, Speed + turn_adjust);

        // 5. 延时
        osDelay(period);
        tick += period;
			
	
    }
	Car_Set_Speed(-Speed, -Speed);
	osDelay(sharpbrake_time);
	Car_Set_Speed(0,0);
}