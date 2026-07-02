/**
 * @filename   : EPD_3in98g.ino
 * @brief      : 3.98寸4色墨水屏 - 图片显示程序
 * @author     : Custom for SEN398HZ07-FNG-A1
 * 
 * 分辨率：552 x 768
 * 颜色：黑、白、红、黄
 * 
 * 引脚连接：
 * CS      -> GPIO 10
 * DC      -> GPIO 11
 * RST     -> GPIO 12
 * BUSY    -> GPIO 13
 * CLK     -> GPIO 14
 * MOSI    -> GPIO 15
 * 
 * 使用说明：
 * 1. 用Img2Lcd生成图片数组，保存为 image_data.h
 * 2. 将此文件放在项目文件夹中
 * 3. 编译上传即可显示图片
 */

#include "DEV_Config.h"
#include "EPD_3in98g.h"

// ============================================================
// 包含图片数据（Img2Lcd生成的文件）
// ============================================================
#include "image_data.h"

// ============================================================
// 分辨率定义
// ============================================================
#define IMAGE_WIDTH    EPD_3IN98G_WIDTH   // 552
#define IMAGE_HEIGHT   EPD_3IN98G_HEIGHT  // 768

// ============================================================
// 颜色定义
// ============================================================
#define COLOR_BLACK   0x0
#define COLOR_WHITE   0x1
#define COLOR_YELLOW  0x2
#define COLOR_RED     0x3

// ============================================================
// 图像缓冲区（用于动态绘制图案时使用）
// ============================================================
#define IMAGE_SIZE     ((IMAGE_HEIGHT / 4) * IMAGE_WIDTH)
UBYTE ImageBuffer[IMAGE_SIZE];

// ============================================================
// 基础绘图函数（用于显示图片前的测试）
// ============================================================

// 设置像素
void SetPixel(int x, int y, UBYTE color) {
    if (x < 0 || x >= IMAGE_WIDTH || y < 0 || y >= IMAGE_HEIGHT) return;
    
    int width_bytes = IMAGE_HEIGHT / 4;
    int byte_index = x * width_bytes + y / 4;
    int bit_offset = (3 - (y % 4)) * 2;
    
    ImageBuffer[byte_index] &= ~(0x03 << bit_offset);
    ImageBuffer[byte_index] |= (color << bit_offset);
}

// 填充矩形
void FillRect(int x, int y, int width, int height, UBYTE color) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            SetPixel(x + j, y + i, color);
        }
    }
}

// 清空缓冲区
void ClearBuffer(UBYTE color) {
    UBYTE packed_color = 0;
    for (int i = 0; i < 4; i++) {
        packed_color |= (color << (i * 2));
    }
    for (int i = 0; i < IMAGE_SIZE; i++) {
        ImageBuffer[i] = packed_color;
    }
}

// ============================================================
// 测试图案（验证屏幕是否正常）
// ============================================================

// 测试1：纯色清屏
void TestClearScreen(void) {
    Serial.println("纯色测试：白色");
    EPD_3IN98G_Clear(COLOR_WHITE);
    delay(2000);
    
    Serial.println("纯色测试：黑色");
    EPD_3IN98G_Clear(COLOR_BLACK);
    delay(2000);
    
    Serial.println("纯色测试：红色");
    EPD_3IN98G_Clear(COLOR_RED);
    delay(2000);
    
    Serial.println("纯色测试：黄色");
    EPD_3IN98G_Clear(COLOR_YELLOW);
    delay(2000);
}

// 测试2：水平条纹
void TestHorizontalStripes(void) {
    Serial.println("测试：水平条纹");
    ClearBuffer(COLOR_WHITE);
    
    int stripe_height = 50;
    for (int y = 0; y < IMAGE_HEIGHT; y++) {
        UBYTE color;
        switch((y / stripe_height) % 4) {
            case 0: color = COLOR_BLACK; break;
            case 1: color = COLOR_RED; break;
            case 2: color = COLOR_YELLOW; break;
            default: color = COLOR_WHITE; break;
        }
        for (int x = 0; x < IMAGE_WIDTH; x++) {
            SetPixel(x, y, color);
        }
    }
    EPD_3IN98G_Display(ImageBuffer);
    delay(3000);
}

// 测试3：垂直条纹
void TestVerticalStripes(void) {
    Serial.println("测试：垂直条纹");
    ClearBuffer(COLOR_WHITE);
    
    int stripe_width = 50;
    for (int x = 0; x < IMAGE_WIDTH; x++) {
        UBYTE color;
        switch((x / stripe_width) % 4) {
            case 0: color = COLOR_BLACK; break;
            case 1: color = COLOR_RED; break;
            case 2: color = COLOR_YELLOW; break;
            default: color = COLOR_WHITE; break;
        }
        for (int y = 0; y < IMAGE_HEIGHT; y++) {
            SetPixel(x, y, color);
        }
    }
    EPD_3IN98G_Display(ImageBuffer);
    delay(3000);
}

// 测试4：四色棋盘格
void TestChessboard(void) {
    Serial.println("测试：四色棋盘格");
    ClearBuffer(COLOR_WHITE);
    
    int block_size = 40;
    for (int y = 0; y < IMAGE_HEIGHT; y += block_size) {
        for (int x = 0; x < IMAGE_WIDTH; x += block_size) {
            int color_index = ((x / block_size) + (y / block_size)) % 4;
            UBYTE color;
            switch(color_index) {
                case 0: color = COLOR_BLACK; break;
                case 1: color = COLOR_RED; break;
                case 2: color = COLOR_YELLOW; break;
                default: color = COLOR_WHITE; break;
            }
            FillRect(x, y, block_size, block_size, color);
        }
    }
    EPD_3IN98G_Display(ImageBuffer);
    delay(3000);
}

// 测试5：矩形边框
void TestRectangle(void) {
    Serial.println("测试：矩形边框");
    ClearBuffer(COLOR_WHITE);
    
    int margin = 50;
    for (int x = margin; x < IMAGE_WIDTH - margin; x++) {
        SetPixel(x, margin, COLOR_BLACK);
        SetPixel(x, IMAGE_HEIGHT - margin, COLOR_BLACK);
    }
    for (int y = margin; y < IMAGE_HEIGHT - margin; y++) {
        SetPixel(margin, y, COLOR_BLACK);
        SetPixel(IMAGE_WIDTH - margin, y, COLOR_BLACK);
    }
    EPD_3IN98G_Display(ImageBuffer);
    delay(3000);
}

// ============================================================
// 图片显示函数
// ============================================================

// 显示Img2Lcd生成的图片
void DisplayImage(const UBYTE* imageData) {
    Serial.println("正在显示图片...");
    EPD_3IN98G_Display(imageData);
    Serial.println("图片显示完成");
}

// ============================================================
// 初始化
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n========================================");
    Serial.println("3.98寸4色墨水屏 - 图片显示程序");
    Serial.printf("分辨率: %d x %d\n", IMAGE_WIDTH, IMAGE_HEIGHT);
    Serial.println("========================================");
    
    // 1. 初始化硬件
    DEV_Config_Init();
    Serial.println("硬件初始化完成");
    
    // 2. 初始化屏幕
    Serial.println("正在初始化屏幕...");
    EPD_3IN98G_Init();
    Serial.println("屏幕初始化完成");
    
    // ============================================================
    // 模式选择：取消注释即可运行对应模式
    // ============================================================
    
    // === 模式1：仅显示图片（正式使用）===
    // 直接显示Img2Lcd生成的图片
    /*
    DisplayImage(gImage_1);  // 请确认数组名与image_data.h中一致
    */
    
    // === 模式2：测试模式（先测试再显示图片）===
    // 取消下面注释，会先运行测试图案，再显示图片
    /*
    TestClearScreen();      // 纯色测试
    TestHorizontalStripes(); // 水平条纹
    TestVerticalStripes();   // 垂直条纹
    TestChessboard();        // 棋盘格
    TestRectangle();         // 矩形边框
    
    delay(2000);
    DisplayImage(gImage_552x768);  // 显示图片
    */
    
    // === 模式3：图片循环显示（需要多张图片）===
    // 取消下面注释，可循环显示多张图片
    
    while(1) {
        DisplayImage(gImage_1);
        delay(50000);
        DisplayImage(gImage_2);
        delay(50000);
        DisplayImage(gImage_3);
        delay(50000);
        DisplayImage(gImage_4);
        delay(50000);
        DisplayImage(gImage_5);
        delay(50000);
    }
    
    
    // 显示完成后进入睡眠模式（可选）
    Serial.println("\n========================================");
    Serial.println("显示完成！屏幕将在10秒后进入睡眠模式");
    Serial.println("========================================");
    delay(10000);
    
    // 进入睡眠模式
    EPD_3IN98G_Sleep();
    Serial.println("屏幕已进入睡眠模式");
}

// ============================================================
// 主循环
// ============================================================
void loop() {
    delay(1000);
}