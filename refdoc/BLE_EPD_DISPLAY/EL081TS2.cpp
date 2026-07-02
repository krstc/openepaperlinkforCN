// EL081TS2.cpp
#include "EL081TS2.h"

EL081TS2::EL081TS2() {
  model = "EL081TS2";
  width = 1024;
  height = 576;
  colorMode = 4;
  cascade = true;
  allScreenBytes = width * height / 4;
}

EL081TS2::~EL081TS2() {
}

void EL081TS2::init() {
  reset();
  checkbusy();
  WriteCMD(TRES);  // Set Panel Reslution
  WriteDATA((width / 2) / 256);
  WriteDATA((width / 2) % 256);
  WriteDATA(height / 256);
  WriteDATA(height % 256);
  WriteCMD(CCSET);  // Enable Cascade Mode
  WriteDATA(0x01);
}

void EL081TS2::writeToRAM(String rxValue, unsigned int *dataIndex, unsigned int dataLength) {
  if (*dataIndex == 0) {
    init();         // 第一次接收到数据时初始化EPD
    WriteCMD(DTM);  // 准备传输数据到EPD缓存
    Serial.println("初始化EPD");
  }
  if (dataLength > 0) {
    unsigned char buff[dataLength];
    memcpy(buff, rxValue.c_str(), dataLength);
    for (int i = 0; i < dataLength; i++)  // 传输接收到的数据，BLE每次传输最大512字节
    {
      if (cascade)  // 如果是级联模式则分开发送数据
      {
        if ((*dataIndex + i) & 0x80U)  // 判断数据发往MasterIC还是SlaveIC
        {
          WriteDATA(buff[i], 2);  //  发到Slave
        } else {
          WriteDATA(buff[i], 1);  //  发到Master
        }
      } else {
        WriteDATA(buff[i]);
      }
    }
    *dataIndex += dataLength;
  }
}