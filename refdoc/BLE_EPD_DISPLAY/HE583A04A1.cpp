// HE583A04A1.cpp
#include "HE583A04A1.h"

HE583A04A1::HE583A04A1() {
  model = "HE583A04A1";
  width = 648;
  height = 480;
  colorMode = 3;
  cascade = false;
  allScreenBytes = width * height / 4;
}

HE583A04A1::~HE583A04A1() {}

void HE583A04A1::init() {
  reset();
  checkbusy();
  WriteCMD(0x01);  //POWER SETTING
  WriteDATA(0x07);
  WriteDATA(0x07);
  WriteDATA(0x3F);
  WriteDATA(0x3F);

  WriteCMD(0x00);  //PANNEL SETTING
  WriteDATA(0x0F);

  WriteCMD(0x61);           //TRES
  WriteDATA(width / 256);   // Source_BITS_H
  WriteDATA(width % 256);   // Source_BITS_L
  WriteDATA(height / 256);  // Gate_BITS_H
  WriteDATA(height % 256);  // Gate_BITS_L

  WriteCMD(0x15);
  WriteDATA(0x00);

  WriteCMD(0x50);  //CDI
  WriteDATA(0x11);
  WriteDATA(0x07);

  WriteCMD(0x60);
  WriteDATA(0x22);

  checkbusy();
}

void HE583A04A1::autoSequence() {
  WriteCMD(PON);
  checkbusy();
  WriteCMD(DRF);
  WriteDATA(0x00);
  checkbusy();
  WriteCMD(POF);
  WriteDATA(0x00);
  checkbusy();
  WriteCMD(DSLP);
  WriteDATA(0xA5);
}

void HE583A04A1::writeToRAM(String rxValue, unsigned int *dataIndex, unsigned int dataLength) {
  // 1. 输入检查
  if (dataLength == 0 || rxValue.length() < dataLength) {
    return;  // 数据无效
  }

  // 2. 首次调用初始化EPD
  if (*dataIndex == 0) {
    init();
    WriteCMD(DTM);  // 准备写入BW RAM
    Serial.println("初始化EPD，准备写入BW RAM");
  }

  // 3. 检查是否已写入全部数据
  if (*dataIndex >= allScreenBytes) {
    return;  // 数据已完整，无需处理
  }

  const char *buff = rxValue.c_str();  // 直接访问字符串数据
  unsigned int processed = 0;

  // 4. 写入前一半数据到BW RAM
  if (*dataIndex < allScreenBytes / 2) {
    unsigned int bwRemaining = (allScreenBytes / 2) - *dataIndex;
    unsigned int toProcess = min(bwRemaining, dataLength);

    for (unsigned int i = 0; i < toProcess; i++) {
      WriteDATA(buff[i]);
      (*dataIndex)++;
      processed++;
    }

    // 如果BW RAM已满，切换到RW RAM
    if (*dataIndex == allScreenBytes / 2) {
      WriteCMD(DTMR);
      Serial.println("切换至RW RAM");
    }
  }

  // 5. 写入后一半数据到RW RAM（取反）
  if (*dataIndex >= allScreenBytes / 2 && processed < dataLength) {
    unsigned int rwRemaining = allScreenBytes - *dataIndex;
    unsigned int toProcess = min(rwRemaining, dataLength - processed);

    for (unsigned int i = processed; i < processed + toProcess; i++) {
      WriteDATA(~buff[i]);  // 数据取反写入
      (*dataIndex)++;
    }
  }
}