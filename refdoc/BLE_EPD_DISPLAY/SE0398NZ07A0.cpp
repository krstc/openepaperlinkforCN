#include "SE0398NZ07A0.h"

SE0398NZ07A0::SE0398NZ07A0() {
  model = "SE0398NZ07A0";
  width = 768;
  height = 552;
  colorMode = 4;
  cascade = false;
  allScreenBytes = width * height / 4;
  linePos = 0;
}

SE0398NZ07A0::~SE0398NZ07A0() {}

void SE0398NZ07A0::init() {
  reset();

  WriteCMD(0x00);
  WriteDATA(0x0B);

  WriteCMD(TRES);  // Set Panel Resolution
  WriteDATA(width / 256);
  WriteDATA(width % 256);
  WriteDATA(height / 256);
  WriteDATA(height % 256);
}

void SE0398NZ07A0::autoSequence() {
  WriteCMD(0x83);
  WriteDATA(0x00);
  WriteDATA(0x00);
  WriteDATA((width - 1) / 256);
  WriteDATA((width - 1) % 256);
  WriteDATA(0x00);
  WriteDATA(0x00);
  WriteDATA((height - 1) / 256);
  WriteDATA((height - 1) % 256);
  WriteDATA(0x01);

  WriteCMD(PON);
  checkbusy();
  uint32_t time = millis();
  WriteCMD(DRF);
  WriteDATA(0x00);
  checkbusy();
  Serial.printf("Refresh Complete in:%dms\n",millis()-time);
  WriteCMD(POF);
  WriteDATA(0x00);
  checkbusy();
  WriteCMD(DSLP);
  WriteDATA(0xA5);
}

// 设置单行窗口（起始行 = physicalRow，结束行 = physicalRow）
void SE0398NZ07A0::setWriteRow(uint16_t physicalRow) {
  WriteCMD(0x83);  // PTL

  // 1st: PTH_ENB=0, HRST[9:8]=0
  WriteDATA(0x00);
  // 2nd: HRST[7:2]
  WriteDATA(0x00);
  // 3rd: HRED[9:8]
  WriteDATA((width - 1) / 256);
  // 4th: HRED[7:2]
  WriteDATA((width - 1) % 256);
  // 5th: VRST[9:8]
  WriteDATA(physicalRow / 256);
  // 6th: VRST[7:0]
  WriteDATA(physicalRow % 256);
  // 7th: VRED[9:8]
  WriteDATA(physicalRow / 256);
  // 8th: VRED[7:0]
  WriteDATA(physicalRow % 256);
  // 9th: PMODE=1 (enable partial mode)
  WriteDATA(0x01);
}

void SE0398NZ07A0::writeToRAM(String rxValue, unsigned int* dataIndex, unsigned int dataLength) {
  if (*dataIndex == 0) {
    init();       // 第一次接收到数据时初始化EPD
    linePos = 0;  // 重置行缓冲区
    Serial.println("初始化EPD");
  }
  if (dataLength == 0) return;

  const unsigned char* pData = (const unsigned char*)rxValue.c_str();
  for (unsigned int i = 0; i < dataLength; i++) {
    lineBuffer[linePos++] = pData[i];
    (*dataIndex)++;

    // 当一行数据填满（192字节）时，执行转换与写入
    if (linePos == 192) {
      // 当前完成的图像行号 = (总字节数/192) - 1
      uint16_t imageRow = (*dataIndex / 192) - 1;
      // 物理行映射
      uint16_t physicalRow;
      if (imageRow < 276) {
        // 上半部分 → 偶数物理行 0,2,4,...,550
        physicalRow = imageRow * 2;
      } else {
        // 下半部分 → 奇数物理行 551,549,...,1
        physicalRow = 551 - 2 * (imageRow - 276);
      }
      // 设置单行窗口
      setWriteRow(physicalRow);
      // 写入当前行数据
      WriteCMD(DTM);  // 准备传输数据到EPD缓存
      for (int j = 0; j < 192; j++) {
        WriteDATA(lineBuffer[j]);
      }
      linePos = 0;  // 重置缓冲区
    }
  }
}