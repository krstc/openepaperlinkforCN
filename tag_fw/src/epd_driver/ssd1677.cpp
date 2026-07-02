#include <Arduino.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drawing.h"
#include "settings.h"
#include "hal.h"
#include "wdt.h"

#include "ssd1677.h"

#define CMD_DRV_OUTPUT_CTRL 0x01
#define CMD_ENTER_SLEEP 0x10
#define CMD_DATA_ENTRY_MODE 0x11
#define CMD_SOFT_RESET 0x12
#define CMD_TEMP_SENSOR_CONTROL 0x18
#define CMD_ACTIVATION 0x20
#define CMD_DISP_UPDATE_CTRL 0x21
#define CMD_DISP_UPDATE_CTRL2 0x22
#define CMD_WRITE_FB_BW 0x24
#define CMD_WRITE_FB_RED 0x26
#define CMD_BORDER_WAVEFORM_CTRL 0x3C
#define CMD_WINDOW_X_SIZE 0x44
#define CMD_WINDOW_Y_SIZE 0x45
#define CMD_XSTART_POS 0x4E
#define CMD_YSTART_POS 0x4F

static void invertBuffer(uint8_t *buf, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        buf[i] ^= 0xFF;
    }
}

void ssd1677::setWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    uint16_t xe = x + w - 1;
    uint16_t ye = y + h - 1;

    epdWrite(CMD_DATA_ENTRY_MODE, 1, 0x03);
    epdWrite(CMD_WINDOW_X_SIZE, 4, x & 0xFF, x >> 8, xe & 0xFF, xe >> 8);
    epdWrite(CMD_WINDOW_Y_SIZE, 4, y & 0xFF, y >> 8, ye & 0xFF, ye >> 8);
    setRamPointer(x, y);
}

void ssd1677::setRamPointer(uint16_t x, uint16_t y) {
    epdWrite(CMD_XSTART_POS, 2, x & 0xFF, x >> 8);
    epdWrite(CMD_YSTART_POS, 2, y & 0xFF, y >> 8);
}

void ssd1677::epdEnterSleep() {
    epdWrite(CMD_ENTER_SLEEP, 1, 0x01);
    delay(100);
}

void ssd1677::epdSetup() {
#ifdef DEBUG_EPD
    printf("Starting SSD1677 EPD Setup\n");
#endif
    epdReset(EPD_BUSY_SSD);
    epdWrite(CMD_SOFT_RESET, 0);
    epdBusyWaitFalling(200);
    epdWrite(CMD_DRV_OUTPUT_CTRL, 3, (this->effectiveYRes - 1) & 0xFF, (this->effectiveYRes - 1) >> 8, 0x00);
    epdWrite(CMD_BORDER_WAVEFORM_CTRL, 1, 0x01);
    epdWrite(CMD_TEMP_SENSOR_CONTROL, 1, 0x80);
    setWindow(0, 0, this->effectiveXRes, this->effectiveYRes);
}

void ssd1677::selectLUT(uint8_t lut) {
    lut += 1;
    wdt120s();
    return;
}

void ssd1677::writePlane(uint8_t cmd, uint8_t color) {
    uint16_t byteWidth = (this->effectiveXRes + 7) / 8;
    uint8_t *buf[2] = {0, 0};

    setRamPointer(0, 0);
    epd_cmd(cmd);
    markData();
    epdSelect();

    for (uint16_t curY = 0; curY < this->effectiveYRes; curY += 2) {
        wdt60s();

        buf[0] = (uint8_t *)calloc(byteWidth, 1);
        drawItem::renderDrawLine(buf[0], curY, color);
        if (this->epdMirrorH) drawItem::reverseBytes(buf[0], byteWidth);
        invertBuffer(buf[0], byteWidth);

        if (buf[1]) {
            epdSPIWait();
            free(buf[1]);
        }
        epdSPIAsyncWrite(buf[0], byteWidth);

        buf[1] = (uint8_t *)calloc(byteWidth, 1);
        if ((curY + 1) < this->effectiveYRes) {
            drawItem::renderDrawLine(buf[1], curY + 1, color);
            if (this->epdMirrorH) drawItem::reverseBytes(buf[1], byteWidth);
        }
        invertBuffer(buf[1], byteWidth);

        epdSPIWait();
        free(buf[0]);
        epdSPIAsyncWrite(buf[1], byteWidth);
    }

    epdSPIWait();
    epdDeselect();
    if (buf[1]) free(buf[1]);
}

void ssd1677::epdWriteDisplayData() {
#ifdef DEBUG_EPD
    printf("SSD1677: Render Start\n");
    uint32_t t_start = millis();
#endif
    setWindow(0, 0, this->effectiveXRes, this->effectiveYRes);
    writePlane(CMD_WRITE_FB_BW, 0);
    if (tag.thirdColor) {
        writePlane(CMD_WRITE_FB_RED, 1);
    } else {
        writePlane(CMD_WRITE_FB_RED, 0);
    }
    drawItem::flushDrawItems();

#ifdef DEBUG_EPD
    printf("SSD1677: Render done in %lu ms\n", millis() - t_start);
#endif
}

void ssd1677::draw() {
    drawNoWait();
    getVoltage();
#ifdef DEBUG_EPD
    printf("SSD1677: Waiting for draw to finish...\n");
#endif
    epdWaitRdy();
#ifdef DEBUG_EPD
    printf("SSD1677: Draw finished\n");
#endif
}

void ssd1677::drawNoWait() {
#ifdef DEBUG_EPD
    printf("SSD1677: Starting to send image data\n");
#endif
    epdWriteDisplayData();
    epdWrite(CMD_DISP_UPDATE_CTRL, 2, tag.thirdColor ? 0x80 : 0x40, 0x00);
    epdWrite(CMD_DISP_UPDATE_CTRL2, 1, 0xF7);
    epdWrite(CMD_ACTIVATION, 0);
#ifdef DEBUG_EPD
    printf("SSD1677: Send Data End\n");
#endif
}

void ssd1677::epdWaitRdy() {
    epdBusyWaitFalling(120000);
    setWindow(0, 0, this->effectiveXRes, this->effectiveYRes);
}
