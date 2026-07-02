#ifndef EPD_3IN98G_H
#define EPD_3IN98G_H

#include "EPD.h"

class EPD_3in98g : public EPD {
public:
  EPD_3in98g();
  ~EPD_3in98g();
  void init() override;
  void autoSequence() override;
  void writeToRAM(String rxValue, unsigned int* dataIndex, unsigned int dataLength) override;

private:
  static const uint16_t BYTES_PER_LINE = 192; // 768像素 ÷ 4 = 192字节/行
  uint8_t lineBuffer[BYTES_PER_LINE];   // 当前行数据缓冲区
  uint16_t linePos;          // 当前行已接收字节数 (0~BYTES_PER_LINE)
  void setWriteRow(uint16_t physicalRow);  // 设置单行窗口
};

#endif