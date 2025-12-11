#include "line_tracker.h"
#include "math.h"   
#include "stdlib.h" 

/* ================= 参数配置区 ================= */

/* 1. 基础速度 & 弯道参数 (保持你原有的值) */
#define MAX_BASE_SPEED      20   
#define SPEED_DROP_FACTOR   8   
#define MOTOR_DEAD_ZONE     18
#define PID_KP              5.0f   
#define PID_KD              10.0f   

/* 2. 路口转向参数 (新增) */
#define TURN_SPEED          25   // 路口转向时的速度
#define TURN_DURATION_MS    400  // 盲转持续时间(ms)，需根据车速调整，确保转出十字路口

/* ============================================ */

/* 传感器引脚读取宏 (保持不变) */
#define READ_L2  (HAL_GPIO_ReadPin(LINE_TRACKER_L2_GPIO_PORT, LINE_TRACKER_L2_GPIO_PIN) == GPIO_PIN_RESET)
#define READ_L1  (HAL_GPIO_ReadPin(LINE_TRACKER_L1_GPIO_PORT, LINE_TRACKER_L1_GPIO_PIN) == GPIO_PIN_RESET)
#define READ_R1  (HAL_GPIO_ReadPin(LINE_TRACKER_R1_GPIO_PORT, LINE_TRACKER_R1_GPIO_PIN) == GPIO_PIN_RESET)
#define READ_R2  (HAL_GPIO_ReadPin(LINE_TRACKER_R2_GPIO_PORT, LINE_TRACKER_R2_GPIO_PIN) == GPIO_PIN_RESET)

/* 
 * [新增] FreeRTOS 对象引用 
 * 请确保在 main.c 或 freertos.c 中定义了这两个句柄
 */
// 视觉任务句柄 (用于唤醒/通知 MVTASK)
extern osThreadId_t MVHandle; 
// 决策队列句柄 (用于接收指令: 1=左, 2=右, 3=直行)
extern osMessageQueueId_t MotorQueueHandle; 

/* PID 数据结构 (保持不变) */
typedef struct {
    float error;       
    float last_error;  
    float output;      
} PID_TypeDef;

static PID_TypeDef line_pid;

/* 初始化 (保持不变) */
void Line_Tracker_Init(void)
{
    line_pid.error = 0;
    line_pid.last_error = 0;
    line_pid.output = 0;
}

/* 死区补偿 (保持不变) */
static int Apply_Dead_Zone(int speed)
{
    if (speed == 0) return 0;
    if (speed > 0) { if (speed < MOTOR_DEAD_ZONE) return MOTOR_DEAD_ZONE; }
    else { if (speed > -MOTOR_DEAD_ZONE) return -MOTOR_DEAD_ZONE; }
    return speed;
}

/* 获取误差 (保持不变) */
static float Get_Line_Error(void)
{
    uint8_t sensor_state = 0;
    static float last_valid_error = 0; 

    if(READ_L2) sensor_state |= 0x08; 
    if(READ_L1) sensor_state |= 0x04; 
    if(READ_R1) sensor_state |= 0x02; 
    if(READ_R2) sensor_state |= 0x01; 

    float current_error = 0;

    switch (sensor_state)
    {
        case 0x06: current_error = 0.0f; break; 
        case 0x09: current_error = 0.0f; break; 
        case 0x02: current_error = 1.0f; break; 
        case 0x03: current_error = 2.0f; break; 
        case 0x01: current_error = 3.0f; break; 
        case 0x07: current_error = 3.5f; break; 
        case 0x04: current_error = -1.0f; break; 
        case 0x0C: current_error = -2.0f; break; 
        case 0x08: current_error = -3.0f; break; 
        case 0x0E: current_error = -3.5f; break; 
        case 0x0F: current_error = 0.0f; break; 
        case 0x00:
            if (last_valid_error > 0) current_error = 4.0f;
            else if (last_valid_error < 0) current_error = -4.0f;
            else current_error = 0.0f;
            break;
        default: current_error = last_valid_error; break;
    }

    if (sensor_state != 0x00) last_valid_error = current_error;
    return current_error;
}

/* [新增] 执行路口盲转动作
 * 目的：根据视觉指令，让小车脱离“全黑”区域，防止死循环
 */
static void Execute_Blind_Turn(uint8_t cmd)
{
    switch (cmd)
    {
        case 1: // 左转 (Left)
            // 左轮反转/不动，右轮正转
            Car_Set_Speed(-TURN_SPEED, TURN_SPEED); 
            break;
        
        case 2: // 右转 (Right)
            // 左轮正转，右轮反转/不动
            Car_Set_Speed(TURN_SPEED, -TURN_SPEED); 
            break;
        
        case 3: // 直行 (Straight)
            Car_Set_Speed(TURN_SPEED, TURN_SPEED); 
            break;
            
        default: // 无效指令，默认停一下
            Car_Set_Speed(0, 0);
            return;
    }
    
    // 延时一段时间让车转过去 (阻塞式延时，此时不进行PID计算)
    osDelay(TURN_DURATION_MS);
    
    // 动作完成后，清除PID的历史误差，准备进入新路段
    Line_Tracker_Init();
}

/**
 * @brief 核心循迹任务 (修改版)
 */
void Line_Tracker_PID_Action(void)
{
    // ================= 1. 路口检测与任务移交 =================
    // 检测 L2 和 R2 同时黑线 (路口特征)
    if (READ_L2 && READ_R2 && !READ_L1 && !READ_R1) 
    {
        // A. 立即停车
		Car_Set_Speed(-20, -20);
		osDelay(100);
        Car_Set_Speed(0, 0);
        
        // B. 唤醒 MVTASK (摄像头任务) 
        
        // C. 挂起当前任务，等待决策消息
        uint8_t turn_cmd = 0;      
//		while(1);
        if (osMessageQueueGet(MotorQueueHandle, &turn_cmd, NULL, osWaitForever) == osOK)
        {
            // D. 收到指令，执行盲转动作 (脱离路口)
            Execute_Blind_Turn(turn_cmd);
            
            return; 
        }
    }
    // ========================================================
    
	
    // ================= 2. 正常 PID 循迹逻辑 =================
    // (以下代码保持你原样，未修改)
	
    // 1. 获取误差
    line_pid.error = Get_Line_Error();

    // 2. 计算 PID (PD算法)
    line_pid.output = (line_pid.error * PID_KP) + ((line_pid.error - line_pid.last_error) * PID_KD);

    // 3. 更新历史误差
    line_pid.last_error = line_pid.error;

    // 4. 计算动态基准速度
    int dynamic_base_speed = MAX_BASE_SPEED - (int)(fabsf(line_pid.error) * SPEED_DROP_FACTOR);
    
    // 限制最小速度不为负
    if (dynamic_base_speed < 0) dynamic_base_speed = 0;

    // 5. 混合计算左右电机目标速度
    int left_motor_target  = dynamic_base_speed + (int)line_pid.output;
    int right_motor_target = dynamic_base_speed - (int)line_pid.output;

    // 6. 死区补偿
    left_motor_target  = Apply_Dead_Zone(left_motor_target);
    right_motor_target = Apply_Dead_Zone(right_motor_target);

    // 7. 下发给电机
    Car_Set_Speed(left_motor_target, right_motor_target);
}
