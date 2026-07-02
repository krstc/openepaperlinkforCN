#ifndef __DEV_CONFIG_H_
#define __DEV_CONFIG_H_

#include <Arduino.h>
#include <SPI.h>

// ============================================================
// 类型定义（关键！）
// ============================================================
typedef unsigned char   UBYTE;
typedef unsigned short  UWORD;
typedef unsigned int    UDOUBLE;

// ============================================================
// 引脚定义（根据你的接线）
// ============================================================
#define EPD_CS_PIN      10
#define EPD_DC_PIN      11
#define EPD_RST_PIN     12
#define EPD_BUSY_PIN    13
#define EPD_SCK_PIN     14
#define EPD_MOSI_PIN    15

// ============================================================
// 外部SPI实例声明
// ============================================================
extern SPIClass hspi;

// ============================================================
// 宏定义 - 硬件操作
// ============================================================
#define DEV_Digital_Write(pin, value) digitalWrite(pin, value)
#define DEV_Digital_Read(pin) digitalRead(pin)
#define DEV_Delay_ms(ms) delay(ms)

// SPI数据传输
#define DEV_SPI_WriteByte(data) hspi.transfer(data)

// 调试输出（启用串口调试）
#define Debug(...) Serial.print(__VA_ARGS__)

// 函数声明
void DEV_Config_Init(void);

#endif