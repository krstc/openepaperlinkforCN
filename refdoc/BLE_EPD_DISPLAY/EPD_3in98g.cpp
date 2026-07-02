#include "EPD_3in98g.h"

EPD_3in98g::EPD_3in98g() {
  model = "EPD_3in98g";
  width = 768;  // 调换为与SE0398NZ07A0相同的宽度768
  height = 552; // 调换为与SE0398NZ07A0相同的高度552
  colorMode = 4;
  cascade = false;
  allScreenBytes = width * height / 4;
  linePos = 0;
  Serial.printf("EPD_3in98g初始化: %dx%d, 总字节数: %u\n", width, height, allScreenBytes);
}

EPD_3in98g::~EPD_3in98g() {}

void EPD_3in98g::init() {
  reset();
  checkbusy();
  delay(30);

  WriteCMD(0xAA);
  WriteDATA(0x49);
  WriteDATA(0x55);
  WriteDATA(0x20);
  WriteDATA(0x08);
  WriteDATA(0x09);
  WriteDATA(0x18);

  WriteCMD(0x01);
  WriteDATA(0x3F);

  WriteCMD(0x00);
  WriteDATA(0x4B);  // 恢复原始扫描方向设置
  WriteDATA(0x69);

  WriteCMD(0x05);
  WriteDATA(0x40);
  WriteDATA(0x1F);
  WriteDATA(0x1F);
  WriteDATA(0x2C);

  WriteCMD(0x08);
  WriteDATA(0x6F);
  WriteDATA(0x1F);
  WriteDATA(0x1F);
  WriteDATA(0x22);

  WriteCMD(0x06);
  WriteDATA(0x6F);
  WriteDATA(0x1F);
  WriteDATA(0x14);
  WriteDATA(0x14);

  WriteCMD(0x03);
  WriteDATA(0x00);
  WriteDATA(0x54);
  WriteDATA(0x00);
  WriteDATA(0x44);

  WriteCMD(0x60);
  WriteDATA(0x02);
  WriteDATA(0x00);
  
  WriteCMD(0x30);
  WriteDATA(0x08);

  WriteCMD(0x50);
  WriteDATA(0x3F);

  WriteCMD(0x61);
  WriteDATA(0x03);
  WriteDATA(0x00);
  WriteDATA(0x02);
  WriteDATA(0x28);

  WriteCMD(0x65);
  WriteDATA(0x10);
  WriteDATA(0x00);
  WriteDATA(0x20);
  WriteDATA(0x00);

  WriteCMD(0xE3);
  WriteDATA(0x2F);

  WriteCMD(0x84);
  WriteDATA(0x01);
}

void EPD_3in98g::autoSequence() {
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

  WriteCMD(0x04);
  checkbusy();
  
  uint32_t time = millis();
  WriteCMD(0x12);
  WriteDATA(0x00);
  checkbusy();
  
  Serial.printf("Refresh Complete in:%dms\n", millis() - time);
  
  WriteCMD(0x02);
  WriteDATA(0x00);
  checkbusy();
  
  WriteCMD(0x07);
  WriteDATA(0xA5);
}

void EPD_3in98g::setWriteRow(uint16_t physicalRow) {
  WriteCMD(0x83);
  WriteDATA(0x00);
  WriteDATA(0x00);
  WriteDATA((width - 1) / 256);
  WriteDATA((width - 1) % 256);
  WriteDATA(physicalRow / 256);
  WriteDATA(physicalRow % 256);
  WriteDATA(physicalRow / 256);
  WriteDATA(physicalRow % 256);
  WriteDATA(0x01);
}

void EPD_3in98g::writeToRAM(String rxValue, unsigned int* dataIndex, unsigned int dataLength) {
  Serial.printf("writeToRAM调用: dataLength=%u, *dataIndex=%u, allScreenBytes=%u, BYTES_PER_LINE=%u\n", 
                dataLength, *dataIndex, allScreenBytes, BYTES_PER_LINE);
  
  if (*dataIndex == 0) {
    Serial.println("第一次调用writeToRAM，初始化EPD...");
    init();       // 第一次接收到数据时初始化EPD
    linePos = 0;  // 重置行缓冲区
    Serial.println("初始化EPD_3in98g完成");
  }
  
  if (dataLength == 0) {
    Serial.println("dataLength为0，直接返回");
    return;
  }

  Serial.printf("开始处理数据: linePos=%u\n", linePos);
  const unsigned char* pData = (const unsigned char*)rxValue.c_str();
  for (unsigned int i = 0; i < dataLength; i++) {
    lineBuffer[linePos++] = pData[i];
    (*dataIndex)++;

    // 当一行数据填满（138字节）时，执行转换与写入
    if (linePos == BYTES_PER_LINE) {
      // 当前完成的图像行号 = (总字节数/BYTES_PER_LINE) - 1
      uint16_t imageRow = (*dataIndex / BYTES_PER_LINE) - 1;
      
      // 物理行映射：使用EPD_3in98g-TU原来的顺序映射
      uint16_t physicalRow = imageRow;

      Serial.printf("行缓冲区已满: imageRow=%u, physicalRow=%u\n", imageRow, physicalRow);
      
      // 设置单行窗口
      setWriteRow(physicalRow);
      
      // 发送数据到RAM
      WriteCMD(0x10);  // 使用0x10命令传输数据，与EPD_3in98g-TU保持一致
      for (int j = 0; j < BYTES_PER_LINE; j++) {
        WriteDATA(lineBuffer[j]);
      }
      
      linePos = 0; // 重置行缓冲区
      Serial.printf("已写入一行数据到RAM，当前总字节数: %u\n", *dataIndex);
    }
  }

  // 如果所有数据都已接收完成，只记录日志，不刷新显示
  if (*dataIndex >= allScreenBytes) {
    Serial.printf("所有数据接收完成，总字节数: %u，等待外部刷新显示\n", *dataIndex);
    // 注意：不在此处刷新显示，由外部调用autoSequence()完成
    // 也不重置dataIndex，由外部函数管理
  }
}