// EPD.h
#ifndef EPD_H
#define EPD_H

#include <Arduino.h>
#include <SPI.h>

// 色彩映射(四色)
#define BLACK 0x00   /// 00
#define WHITE 0x01   /// 01
#define YELLOW 0x02  /// 10
#define RED 0x03     /// 11

//Common Command Table for JD796XX/UC81XX/SSD2677
#define PSR 0x00
#define PWR 0x01
#define POF 0x02
#define PON 0x04
#define DSLP 0x07
#define DTM 0x10
#define DSP 0x11
#define DRF 0x12
#define DTMR 0x13
#define AUTO 0x17
#define TRES 0x61
#define CCSET 0xE0

// EPD SPI 引脚定义 - 根据EPD_3in98g-TU配置

#define EPD_SCL 14  // SCK
#define EPD_SDA 15  // MOSI
#define EPD_CS 10
#define EPD_CS_2 10  // 单CS模式
#define EPD_DC 11
#define EPD_BUSY 13
#define EPD_RST 12
/*
// 备用配置
#define EPD_SCL 18
#define EPD_SDA 19
#define EPD_CS 5
#define EPD_CS_2 15
#define EPD_DC 17
#define EPD_BUSY 4
#define EPD_RST 16
*/
class EPD {
public:
  EPD();
  virtual ~EPD();
  virtual void checkbusy(unsigned long timeout_ms = 50000);
  virtual bool state();
  virtual void reset();
  virtual void init() = 0;
  virtual void deepsleep();
  virtual void refresh();
  virtual void poweron();
  virtual void poweroff();
  virtual void autoSequence();
  virtual void fillscreen(unsigned char color = WHITE);
  virtual void drawBitmap(const unsigned char *bitmap);
  virtual void writeToRAM(String rxValue, unsigned int *dataIndex, unsigned int dataLength) = 0;

  const char *model;
  int width;
  int height;
  int colorMode;
  bool cascade;
  uint32_t allScreenBytes;

protected:
  void spi_init();
  void WriteCMD(unsigned char command, unsigned char cs = 0);
  void WriteDATA(unsigned char data, unsigned char cs = 0);
  void SPI_Write(unsigned char value);
};

#endif