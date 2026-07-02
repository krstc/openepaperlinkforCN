#include <Arduino.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hal.h"
#include "lut.h"
#include "settings.h"
#include "wdt.h"
#include "drawing.h"

#include "uc8159.h"

#define CMD_PANEL_SETTING 0x00
#define CMD_POWER_SETTING 0x01
#define CMD_POWER_OFF 0x02
#define CMD_POWER_OFF_SEQUENCE 0x03
#define CMD_POWER_ON 0x04
#define CMD_BOOSTER_SOFT_START 0x06
#define CMD_DEEP_SLEEP 0x07
#define CMD_DISPLAY_START_TRANSMISSION_DTM1 0x10
#define CMD_DATA_STOP 0x11
#define CMD_DISPLAY_REFRESH 0x12
#define CMD_DISPLAY_IMAGE_PROCESS 0x13
#define CMD_VCOM_LUT_C 0x20
#define CMD_LUT_B 0x21
#define CMD_LUT_W 0x22
#define CMD_LUT_G1 0x23
#define CMD_LUT_G2 0x24
#define CMD_LUT_R0 0x25
#define CMD_LUT_R1 0x26
#define CMD_LUT_R2 0x27
#define CMD_LUT_R3 0x28
#define CMD_LUT_XON 0x29
#define CMD_PLL_CONTROL 0x30
#define CMD_TEMPERATURE_DOREADING 0x40
#define CMD_TEMPERATURE_SELECT 0x41
#define CMD_TEMPERATURE_WRITE 0x42
#define CMD_TEMPERATURE_READ 0x43
#define CMD_VCOM_INTERVAL 0x50
#define CMD_LOWER_POWER_DETECT 0x51
#define CMD_TCON_SETTING 0x60
#define CMD_RESOLUTION_SETING 0x61
#define CMD_SPI_FLASH_CONTROL 0x65
#define CMD_REVISION 0x70
#define CMD_STATUS 0x71
#define CMD_AUTO_MEASUREMENT_VCOM 0x80
#define CMD_READ_VCOM 0x81
#define CMD_VCOM_DC_SETTING 0x82
#define CMD_PARTIAL_WINDOW 0x90
#define CMD_PARTIAL_IN 0x91
#define CMD_PARTIAL_OUT 0x92
#define CMD_PROGRAM_MODE 0xA0
#define CMD_ACTIVE_PROGRAM 0xA1
#define CMD_READ_OTP 0xA2
#define CMD_EPD_EEPROM_SLEEP 0xB9
#define CMD_EPD_EEPROM_WAKE 0xAB
#define CMD_CASCADE_SET 0xE0
#define CMD_POWER_SAVING 0xE3
#define CMD_FORCE_TEMPERATURE 0xE5
#define CMD_LOAD_FLASH_LUT 0xE5

#define UC8159_PSR_RES_640_384 0xCF
#define UC8159_PSR_BWR 0x08
#define UC8159_FIXED_PLL 0x3A
#define UC8159_FIXED_VCOM 0x28
#define UC8159_FLASH_LUT 0x03

#define epdEepromSelect()           \
    do {                            \
        digitalWrite(EPD_HLT, LOW); \
    } while (0)

#define epdEepromDeselect()          \
    do {                             \
        digitalWrite(EPD_HLT, HIGH); \
    } while (0)

void dump(const uint8_t *a, const uint16_t l);

static uint8_t uc8159PixelColor(uint8_t b, uint8_t r, uint8_t mask) {
    if (r & mask) {
        return 0x04;
    }
    if (b & mask) {
        return 0x00;
    }
    return 0x03;
}

void uc8159::epdEepromRead(uint16_t addr, uint8_t *data, uint16_t len) {
    // return;
    epdWrite(CMD_SPI_FLASH_CONTROL, 1, 0x01);
    delay(1);
    epdEepromSelect();
    spi_write(0x03);  // EEPROM READ;
    spi_write(0x00);
    spi_write(addr >> 8);
    spi_write(addr & 0xFF);
    epdSPIReadBlock(data, len);
    epdEepromDeselect();
    delay(1);
    epdWrite(CMD_SPI_FLASH_CONTROL, 1, 0x00);
}
uint8_t uc8159::getTempBracket() {
    uint8_t temptable[10];
    epdEepromRead(25002, temptable, 10);
    epdWrite(CMD_TEMPERATURE_DOREADING, 0);
    epdBusyWaitRising(1500);
    epdHardSPI(false);
    int8_t temp = spi3_read();
    temp <<= 1;
    temp |= (spi3_read() >> 7);

    uint8_t bracket = 0;
    for (int i = 0; i < 9; i++) {
        if ((((char)temp - (uint8_t)temptable[i]) & 0x80) != 0) {
            bracket = i;
            break;
        }
    }
    epdHardSPI(true);
    return bracket;
}
void uc8159::loadFrameRatePLL(uint8_t bracket) {
    uint8_t pllvalue;
    uint8_t plltable[10];
    epdEepromRead(0x6410, plltable, 10);
    pllvalue = plltable[bracket];
    if (!pllvalue) pllvalue = 0x3C;  // check if there's a valid pll value; if not; load preset
    epdWrite(CMD_PLL_CONTROL, 1, pllvalue);
}
void uc8159::loadTempVCOMDC(uint8_t bracket) {
    uint8_t vcomvalue;
    uint8_t vcomtable[10];
    epdEepromRead(25049, vcomtable, 10);
    vcomvalue = vcomtable[bracket];
    if (!vcomvalue) {
        // if we couldn't find the vcom table, then it's a fixed value sitting at 0x6400
        epdEepromRead(0x6400, vcomtable, 10);
        if (vcomtable[0])
            vcomvalue = vcomtable[0];
        else
            vcomvalue = 0x1E;  // check if there's a valid vcomvalue; if not; load preset
    }
    epdWrite(CMD_VCOM_DC_SETTING, 1, vcomvalue);
}

void uc8159::epdEnterSleep() {
    epdWrite(CMD_POWER_OFF, 0);
    epdBusyWaitRising(250);
    epdWrite(CMD_DEEP_SLEEP, 1, 0xA5);
}
void uc8159::epdSetup() {
    epdReset(EPD_BUSY_UC);
    digitalWrite(EPD_BS, LOW);

    epdWrite(CMD_POWER_SETTING, 2, 0x37, 0x00);
    epdWrite(CMD_PANEL_SETTING, 2, UC8159_PSR_RES_640_384, UC8159_PSR_BWR);
    epdWrite(CMD_PLL_CONTROL, 1, UC8159_FIXED_PLL);
    epdWrite(CMD_VCOM_DC_SETTING, 1, UC8159_FIXED_VCOM);
    epdWrite(CMD_BOOSTER_SOFT_START, 3, 0xC7, 0xCC, 0x15);
    epdWrite(CMD_VCOM_INTERVAL, 1, 0x77);
    epdWrite(CMD_TCON_SETTING, 1, 0x22);
    epdWrite(CMD_SPI_FLASH_CONTROL, 1, 0x00);
    epdWrite(CMD_LOAD_FLASH_LUT, 1, UC8159_FLASH_LUT);
    epdWrite(CMD_RESOLUTION_SETING, 4, this->effectiveXRes >> 8, this->effectiveXRes & 0xFF, this->effectiveYRes >> 8, this->effectiveYRes & 0xFF);
    epdWrite(CMD_POWER_ON, 0);
    epdBusyWaitRising(250);
    setWindow(0, 0, this->effectiveXRes, this->effectiveYRes);
}

void uc8159::interleaveColorToBuffer(uint8_t *dst, uint8_t b, uint8_t r) {
    for (uint8_t mask = 0x80; mask; mask >>= 1) {
        uint8_t packed = uc8159PixelColor(b, r, mask) << 4;
        mask >>= 1;
        packed |= uc8159PixelColor(b, r, mask);
        *dst++ = packed;
    }
}

void uc8159::setWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    x &= 0xFFF8;
    uint16_t xe = x + w - 1;
    xe |= 0x0007;
    uint16_t ye = y + h - 1;

    epdWrite(CMD_PARTIAL_WINDOW, 9,
             x >> 8, x & 0xFF,
             xe >> 8, xe & 0xFF,
             y >> 8, y & 0xFF,
             ye >> 8, ye & 0xFF,
             0x00);
}

void uc8159::selectLUT(uint8_t lut) {
    // implement alternative LUTs here. Currently just reset the watchdog to two minutes,
    // to ensure it doesn't reset during the much longer bootup procedure
    lut += 1;  // make the compiler a happy camper
    wdt120s();
    return;
}

void uc8159::epdWriteDisplayData() {
    uint8_t blocksize = 16;
    uint16_t byteWidth = this->effectiveXRes / 8;
    uint8_t screenrow_bw[byteWidth * blocksize];
    uint8_t screenrow_r[byteWidth * blocksize];
    uint8_t screenrowInterleaved[byteWidth * 4];

    epdWrite(CMD_PARTIAL_IN, 0);
    setWindow(0, 0, this->effectiveXRes, this->effectiveYRes);
    epd_cmd(CMD_DISPLAY_START_TRANSMISSION_DTM1);
    markData();
    epdSelect();

    for (uint16_t curY = 0; curY < this->effectiveYRes; curY += blocksize) {  //
        uint8_t rowsThisBlock = ((this->effectiveYRes - curY) < blocksize) ? (this->effectiveYRes - curY) : blocksize;
        wdt30s();
        memset(screenrow_bw, 0, byteWidth * blocksize);
        memset(screenrow_r, 0, byteWidth * blocksize);

        for (uint8_t bcount = 0; bcount < rowsThisBlock; bcount++) {
            drawItem::renderDrawLine(screenrow_bw + (byteWidth * bcount), curY + bcount, 0);
        }
        for (uint8_t bcount = 0; bcount < rowsThisBlock; bcount++) {
            drawItem::renderDrawLine(screenrow_r + (byteWidth * bcount), curY + bcount, 1);
        }

        for (uint8_t bcount = 0; bcount < rowsThisBlock; bcount++) {
            for (uint16_t curX = 0; curX < (byteWidth); curX++) {
                interleaveColorToBuffer(screenrowInterleaved + (curX * 4), screenrow_bw[curX + (byteWidth * bcount)], screenrow_r[curX + (byteWidth * bcount)]);
            }

            epdSPIAsyncWrite(screenrowInterleaved, byteWidth * 4);
            epdSPIWait();
            epdDeselect();
            epdSelect();
        }
    }
    epdSPIWait();

    epdDeselect();
    epd_cmd(CMD_DATA_STOP);
    epdWrite(CMD_PARTIAL_OUT, 0);

    drawItem::flushDrawItems();
}

void uc8159::draw() {
    delay(1);
    drawNoWait();
    epdBusyWaitRising(30000);
}
void uc8159::drawNoWait() {
    epdWriteDisplayData();
    epdWrite(CMD_LOAD_FLASH_LUT, 1, UC8159_FLASH_LUT);
    setWindow(0, 0, this->effectiveXRes, this->effectiveYRes);
    epdWrite(CMD_DISPLAY_REFRESH, 0);
}
void uc8159::epdWaitRdy() {
    epdBusyWaitRising(30000);
}
