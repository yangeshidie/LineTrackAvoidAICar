from machine import UART
from machine import FPIOA
import time
import os,sys
from media.sensor import *
from media.display import *
from media.media import *
import nncase_runtime as nn
import ulab.numpy as np
import time,image,random,gc

# -----------------
# 识别初始化
# -----------------
sensor_id = 2
sensor = None

kmodel_path="/data/arrownet.kmodel"
kpu=nn.kpu()
kpu.load_kmodel(kmodel_path)

img_path="/data/test_arrow_right.png"
img_data = image.Image(img_path)
test_mode = 0

# 显示模式选择：可以是 "VIRT"、"LCD" 或 "HDMI"
DISPLAY_MODE = "LCD"

# 根据模式设置显示宽高
if DISPLAY_MODE == "VIRT":
    # 虚拟显示器模式
    DISPLAY_WIDTH = ALIGN_UP(1920, 16)
    DISPLAY_HEIGHT = 1080
elif DISPLAY_MODE == "LCD":
    # 3.1寸屏幕模式
    DISPLAY_WIDTH = 800
    DISPLAY_HEIGHT = 480
elif DISPLAY_MODE == "HDMI":
    # HDMI扩展板模式
    DISPLAY_WIDTH = 1920
    DISPLAY_HEIGHT = 1080
else:
    raise ValueError("未知的 DISPLAY_MODE，请选择 'VIRT', 'LCD' 或 'HDMI'")
roi_w, roi_h = 200, 200
roi_x = (DISPLAY_WIDTH - roi_w) // 2
roi_y = (DISPLAY_HEIGHT - roi_h) // 2
labels = ["L", "N", "R"]

# -----------------
# 串口初始化
# -----------------
fpioa = FPIOA()
fpioa.set_function(11, FPIOA.UART2_TXD)
fpioa.set_function(12, FPIOA.UART2_RXD)
#fpioa.help()
uart = UART(UART.UART2, baudrate=115200, bits=UART.EIGHTBITS, parity=UART.PARITY_NONE, stop=UART.STOPBITS_ONE)

CMD_COLOR   =   "0"
CMD_TURN    =   "1"
CMD_WAIT    =   "2"

cmd         =   CMD_WAIT
message = "TEST\n"
data = None

# -----------------
# 传感器初始化
# -----------------
sensor = Sensor(id=sensor_id)
# 重置摄像头sensor
sensor.reset()
# 设置通道0的输出尺寸
sensor.set_framesize(width=DISPLAY_WIDTH, height=DISPLAY_HEIGHT, chn=CAM_CHN_ID_0)
# 设置通道0的输出像素格式为RGB888
sensor.set_pixformat(Sensor.RGB888, chn=CAM_CHN_ID_0)
# 根据模式初始化显示器
if DISPLAY_MODE == "VIRT":
    Display.init(Display.VIRT, width=DISPLAY_WIDTH, height=DISPLAY_HEIGHT, fps=60)
elif DISPLAY_MODE == "LCD":
    Display.init(Display.ST7701, width=DISPLAY_WIDTH, height=DISPLAY_HEIGHT, to_ide=True)
elif DISPLAY_MODE == "HDMI":
    Display.init(Display.LT9611, width=DISPLAY_WIDTH, height=DISPLAY_HEIGHT, to_ide=True)
# 初始化媒体管理器
MediaManager.init()
sensor.set_hmirror(True)
# 设置垂直翻转
sensor.set_vflip(True)
# 启动传感器
sensor.run()

# -----------------
# 颜色初始化
# -----------------
color_threshold_red    = [(0, 100, 19, 51, -6, 33)]
color_threshold_green  = [(0, 100, -78, -9, 5, 27)]
color_threshold_blue   = [(0, 100, -24, -8, -128, -1)]
color_threshold_blue_big   = [(25, 41, -24, 23, -42, -12)]
RGB_blobs_num  =  [0,0,0];

def update_command():
    data = None
    while data is None:
        data = uart.read()
        #time.sleep_ms(5)
    #uart.write(data)
    try:
        # 1. data.decode() 将 bytes (b'1') 转换为 str ('1')
        # 2. strip() 去除串口发送可能附带的回车换行符 (\r\n)
        cmd_char = data.decode('utf-8').strip()
    except Exception:
        # 如果接收到的数据无法解码（比如干扰数据），返回空字符串
        cmd_char = ""
    # --- 修改结束 ---

    return cmd_char
def my_resize_togray(src_img, dst_w, dst_h):
        gray_img = src_img
        gray_img = src_img.to_grayscale()

        src_w = gray_img.width()
        src_h = gray_img.height()
        dst_np = np.zeros((dst_h, dst_w), dtype=np.uint8)
        x_ratio = src_w / dst_w
        y_ratio = src_h / dst_h
        for y in range(dst_h):
            for x in range(dst_w):
                src_x = int(x * x_ratio)
                src_y = int(y * y_ratio)
                pixel_value = gray_img.get_pixel(src_x, src_y)
                dst_np[y, x] = pixel_value
        return dst_np

def arrow_identify():
    # 捕获通道0的图像
    result_final = [0,0,0]
    for jklh in range(10):
        img = sensor.snapshot(chn=CAM_CHN_ID_0)
        roi_img = img.crop(roi=(roi_x, roi_y, roi_w, roi_h))
        # 使用您的函数对ROI区域进行预处理
        img_np = my_resize_togray(roi_img, 64, 64)
        del roi_img # 及时释放内存
        # AI推理过程
        runtime_tensor=nn.from_numpy(img_np)
        kpu.set_input_tensor(0,runtime_tensor)
        kpu.run()
        results=[]
        for i in range(kpu.outputs_size()):
            output_i_tensor = kpu.get_output_tensor(i)
            result_i = output_i_tensor.to_numpy()
            results.append(result_i)
            output_i_tensor
        del runtime_tensor
        nn.shrink_memory_pool()
        for i in range(3):
            result_final[i] = results[0][0][i]
        img.draw_rectangle(roi_x, roi_y, roi_w, roi_h, color=(255, 0, 0), thickness=2)
        Display.show_image(img)
        gc.collect()
    prediction_index = np.argmax(result_final)
    result_text = labels[prediction_index]
    img.draw_string_advanced(20, 20, 50, result_text, color=(0, 255, 0), scale=2)
    return result_text

def Find_clors(color_threshold):
    os.exitpoint()
    img = sensor.snapshot(chn=CAM_CHN_ID_0)
    blobs = img.find_blobs(color_threshold,area_threshold = 3000)
    if blobs:
        for blob in blobs:
            img.draw_rectangle(blob[0:4])
            img.draw_cross(blob[5], blob[6])
            # print("Blob Center: X={}, Y={}".format(blob[5], blob[6]))
    Display.show_image(img)
    return len(blobs)

def Find_Three_blobs(threshold_red,threshold_green,threshold_blue):
    RGB_blobs_num[0] = Find_clors(threshold_red)
    RGB_blobs_num[1] = Find_clors(threshold_green)
    RGB_blobs_num[2] = Find_clors(threshold_blue)
try:
    while(True):
        #cmd = update_command()
        cmd = CMD_TURN
        if cmd == CMD_WAIT:
            uart.write("AABBCMDW")
            print("WAIT")
            time.sleep(1)
            #continue
        elif cmd == CMD_COLOR:
            Find_Three_blobs(color_threshold_red,color_threshold_green,color_threshold_blue)
            result = "AABBN"+str(RGB_blobs_num[0])+str(RGB_blobs_num[1])+str(RGB_blobs_num[2])
            uart.write(result)
            print(result)
            cmd = CMD_WAIT
        elif cmd == CMD_TURN:
            text = "AAAABBB"+arrow_identify()
            uart.write(text)
            print(text)
            cmd = CMD_WAIT
        else:
            cmd = CMD_WAIT
except KeyboardInterrupt as e:
    print("用户停止: ", e)
except BaseException as e:
    print(f"异常: {e}")

finally:
    del kpu
    # 停止传感器运行`
    if isinstance(sensor, Sensor):
        sensor.stop()
    # 反初始化显示模块
    Display.deinit()
    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    time.sleep_ms(100)
    # 释放媒体缓冲区
    MediaManager.deinit()
    uart.deinit()
