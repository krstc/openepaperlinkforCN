#ifndef SE0398NZ07A0_H
#define SE0398NZ07A0_H

#include "EPD.h"

class SE0398NZ07A0 : public EPD {
public:
  SE0398NZ07A0();
  ~SE0398NZ07A0();
  void init() override;
  void autoSequence() override;
  void writeToRAM(String rxValue, unsigned int* dataIndex, unsigned int dataLength) override;

private:
  uint8_t lineBuffer[192];   // 当前行数据缓冲区
  uint16_t linePos;          // 当前行已接收字节数 (0~192)
  void setWriteRow(uint16_t physicalRow);  // 设置单行窗口
};

#endif