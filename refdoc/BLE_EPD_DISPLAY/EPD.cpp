// EPD.cpp
#include "EPD.h"

EPD::EPD() {
  spi_init();
}

EPD::~EPD() {}

void EPD::checkbusy(unsigned long timeout_ms) {
  unsigned long start = millis();
  while (digitalRead(EPD_BUSY) == LOW) {
    delay(1);
    if (millis() - start >= timeout_ms) {
      break;
    }
    yield();
  }
}

bool EPD::state() {
  if (digitalRead(EPD_BUSY)) {
    return false;
  } else {
    return true;
  }
}

void EPD::reset() {
  delay(200);
  digitalWrite(EPD_RST, LOW);
  delay(100);
  digitalWrite(EPD_RST, HIGH);
  checkbusy();
}

void EPD::deepsleep() {
  WriteCMD(POF);
  WriteDATA(0x00);
  checkbusy();
  WriteCMD(DSLP);
  WriteDATA(0xA5);
}

void EPD::refresh() {
  WriteCMD(DRF);
  WriteDATA(0x00);
  checkbusy();
}

void EPD::poweron() {
  WriteCMD(PON);
  checkbusy();
}

void EPD::poweroff() {
  WriteCMD(POF);
  WriteDATA(0x00);
  checkbusy();
}

void EPD::autoSequence() {
  Serial.println(digitalRead(EPD_BUSY));
  WriteCMD(PON);
  checkbusy();
  Serial.println(digitalRead(EPD_BUSY));
  WriteCMD(DRF);
  WriteDATA(0x00);
  checkbusy();
  Serial.println(digitalRead(EPD_BUSY));
  WriteCMD(POF);
  WriteDATA(0x00);
  checkbusy();
  Serial.println(digitalRead(EPD_BUSY));
  WriteCMD(DSLP);
  WriteDATA(0xA5);
  Serial.println(digitalRead(EPD_BUSY));
}

void EPD::fillscreen(unsigned char color) {
  color = color > 3 ? 1 : color;
  uint8_t data = 0;
  for (uint8_t i = 0; i < 4; i++) {
    data <<= 2;
    data += color;
  }
  WriteCMD(DTM);
  for (uint32_t i = 0; i < allScreenBytes; i++) {
    WriteDATA(data);
  }
  refresh();
  checkbusy();
}

void EPD::drawBitmap(const unsigned char *bitmap) {
  WriteCMD(DTM);
  uint32_t SIZE = 0;
  if (cascade) {
    SIZE = allScreenBytes * 2;
  } else {
    SIZE = allScreenBytes;
  }
  for (uint32_t i = 0; i < SIZE; i++) {
    if (i & 0x80) {
      WriteDATA(bitmap[i], 2);
    } else {
      WriteDATA(bitmap[i], 1);
    }
  }
  refresh();
  checkbusy();
}

void EPD::spi_init() {
  pinMode(EPD_CS, OUTPUT);
  pinMode(EPD_CS_2, OUTPUT);
  pinMode(EPD_DC, OUTPUT);
  pinMode(EPD_BUSY, INPUT);
  pinMode(EPD_RST, OUTPUT);
  // SPI
  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
  SPI.begin(EPD_SCL, -1, EPD_SDA, -1);
}

void EPD::WriteCMD(unsigned char command, unsigned char cs) {
  switch (cs) {
    case 0:  // Send to both IC
      digitalWrite(EPD_CS, LOW);
      digitalWrite(EPD_CS_2, LOW);
      break;
    case 1:  // Send to Master IC
      digitalWrite(EPD_CS, LOW);
      break;
    case 2:  // Send to Slave IC
      digitalWrite(EPD_CS_2, LOW);
      break;
    default:  // Send to both IC
      digitalWrite(EPD_CS, LOW);
      digitalWrite(EPD_CS_2, LOW);
      break;
  }
  digitalWrite(EPD_DC, LOW);  // D/C#   0:command  1:data
  SPI_Write(command);
  digitalWrite(EPD_CS, HIGH);
  digitalWrite(EPD_CS_2, HIGH);
}

void EPD::WriteDATA(unsigned char data, unsigned char cs) {
  switch (cs) {
    case 0:  // Send to both IC
      digitalWrite(EPD_CS, LOW);
      digitalWrite(EPD_CS_2, LOW);
      break;
    case 1:  // Send to Master IC
      digitalWrite(EPD_CS, LOW);
      break;
    case 2:  // Send to Slave IC
      digitalWrite(EPD_CS_2, LOW);
      break;
    default:  // Send to both IC
      digitalWrite(EPD_CS, LOW);
      digitalWrite(EPD_CS_2, LOW);
      break;
  }
  digitalWrite(EPD_DC, HIGH);  // D/C#   0:command  1:data
  SPI_Write(data);
  digitalWrite(EPD_CS, HIGH);
  digitalWrite(EPD_CS_2, HIGH);
}

void EPD::SPI_Write(unsigned char value) {
  SPI.transfer(value);
}