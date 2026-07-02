#include "DEV_Config.h"

// 创建SPI实例
SPIClass hspi(HSPI);

void DEV_Config_Init(void) {
    // 初始化引脚模式
    pinMode(EPD_CS_PIN, OUTPUT);
    pinMode(EPD_DC_PIN, OUTPUT);
    pinMode(EPD_RST_PIN, OUTPUT);
    pinMode(EPD_BUSY_PIN, INPUT);
    
    // 设置默认电平
    digitalWrite(EPD_CS_PIN, HIGH);
    digitalWrite(EPD_DC_PIN, HIGH);
    digitalWrite(EPD_RST_PIN, HIGH);
    
    // 初始化SPI总线
    hspi.begin(EPD_SCK_PIN, -1, EPD_MOSI_PIN, EPD_CS_PIN);
    hspi.setFrequency(2000000);   // 2MHz
    hspi.setBitOrder(MSBFIRST);
    hspi.setDataMode(SPI_MODE0);
}