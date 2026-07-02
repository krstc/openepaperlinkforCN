//HSE097SE.cpp
#include "HSE097SE.h"

HSE097SE::HSE097SE() {
  model = "HSE097SE";
  width = 960;
  height = 672;
  colorMode = 4;
  cascade = false;
  allScreenBytes = width * height / 4;
}

HSE097SE::~HSE097SE() {}

void HSE097SE::init(void) {
  reset();
  WriteCMD(0x4D);  //LOAD FROM OTP
  WriteDATA(0x78);

  WriteCMD(PSR);  //PSR
  WriteDATA(0x5F);
  WriteDATA(0xA9);

  WriteCMD(0x06);  //BTST_P
  WriteDATA(0x0F);
  WriteDATA(0x8B);
  WriteDATA(0x93);
  WriteDATA(0xA1);

  WriteCMD(0x50);  //CDI
  WriteDATA(0x37);

  WriteCMD(TRES);           //TRES
  WriteDATA(width / 256);   // Source_BITS_H
  WriteDATA(width % 256);   // Source_BITS_L
  WriteDATA(height / 256);  // Gate_BITS_H
  WriteDATA(height % 256);  // Gate_BITS_L

  WriteCMD(0x62);
  WriteDATA(0x77);
  WriteDATA(0x77);
  WriteDATA(0x77);
  WriteDATA(0x5C);
  WriteDATA(0x9F);
  WriteDATA(0x8C);
  WriteDATA(0x77);
  WriteDATA(0x63);

  WriteCMD(0XE7);  // PST
  WriteDATA(0x16);

  WriteCMD(0xE9);
  WriteDATA(0x01);

  WriteCMD(PWR);  //PWRR
  WriteDATA(0x07);
  WriteDATA(E5_WSF[528]);
  WriteDATA(E5_WSF[529]);
  WriteDATA(E5_WSF[531]);
  WriteDATA(E5_WSF[530]);
  WriteDATA(E5_WSF[532]);

  WriteCMD(0x82);
  WriteDATA(E5_WSF[533] + 0x80);

  WriteCMD(0x30);  //PLL
  WriteDATA(E5_WSF[534]);

  WriteCMD(0x20);  //LUT
  int i, j;
  for (i = 528; i < 535; i++) {
    WriteDATA(E5_WSF[i]);
  }
  for (i = 0; i < 48; i++)  //528
  {
    for (j = 0; j < 11; j++) {
      WriteDATA(E5_WSF[i + 48 * j]);
    }
  }
}

void HSE097SE::writeToRAM(String rxValue, unsigned int *dataIndex, unsigned int dataLength) {
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