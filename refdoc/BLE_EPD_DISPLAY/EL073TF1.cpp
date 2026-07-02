// EL073TF1.cpp
#include "EL073TF1.h"

EL073TF1::EL073TF1() {
  model = "EL073TF1";
  width = 800;
  height = 480;
  colorMode = 6;
  cascade = false;
  allScreenBytes = width * height / 2;
}

EL073TF1::~EL073TF1() {}

void EL073TF1::init() {
  reset();
  WriteCMD(0xAA);  // CMDH
  WriteDATA(0x49);
  WriteDATA(0x55);
  WriteDATA(0x20);
  WriteDATA(0x08);
  WriteDATA(0x09);
  WriteDATA(0x18);

  WriteCMD(0x01);  //
  WriteDATA(0x3F);

  WriteCMD(0x00);
  WriteDATA(0x5F);
  WriteDATA(0x69);

  WriteCMD(0x03);
  WriteDATA(0x00);
  WriteDATA(0x54);
  WriteDATA(0x00);
  WriteDATA(0x44);

  WriteCMD(0x05);
  WriteDATA(0x40);
  WriteDATA(0x1F);
  WriteDATA(0x1F);
  WriteDATA(0x2C);

  WriteCMD(0x06);
  WriteDATA(0x6F);
  WriteDATA(0x1F);
  WriteDATA(0x17);
  WriteDATA(0x49);

  WriteCMD(0x08);
  WriteDATA(0x6F);
  WriteDATA(0x1F);
  WriteDATA(0x1F);
  WriteDATA(0x22);
  WriteCMD(0x30);
  WriteDATA(0x08);  // 原始0x03
  WriteCMD(0x50);
  WriteDATA(0x3F);

  WriteCMD(0x60);
  WriteDATA(0x02);
  WriteDATA(0x00);

  WriteCMD(0x61);
  WriteDATA(0x03);
  WriteDATA(0x20);
  WriteDATA(0x01);
  WriteDATA(0xE0);

  WriteCMD(0x84);
  WriteDATA(0x01);

  WriteCMD(0xE3);
  WriteDATA(0x2F);
}

void EL073TF1::writeToRAM(String rxValue, unsigned int *dataIndex, unsigned int dataLength) {
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