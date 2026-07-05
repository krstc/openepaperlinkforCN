#include <Arduino.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drawing.h"
#include "hal.h"
#include "settings.h"
#include "wdt.h"

#include "jd79665.h"
#include "../../../shared/oepl-definitions.h"

#define CMD_PANEL_SETTING 0x00
#define CMD_POWER_OFF 0x02
#define CMD_POWER_ON 0x04
#define CMD_BOOSTER_SOFT_START 0x06
#define CMD_DEEP_SLEEP 0x07
#define CMD_DATA_START 0x10
#define CMD_DISPLAY_REFRESH 0x12
#define CMD_PLL_CONTROL 0x30
#define CMD_VCOM_INTERVAL 0x50
#define CMD_TCON_SETTING 0x60
#define CMD_RESOLUTION_SETTING 0x61
#define CMD_GSST_SETTING 0x65
#define CMD_PARTIAL_WINDOW 0x83

#define JD79665_NEW_GATE_GAP 24

bool jd79665::waitReady(uint32_t timeout) {
    uint32_t start = millis();
    uint32_t lastWdt = start;

    wdt120s();
    delay(1);
    while (millis() - start < timeout) {
        if (digitalRead(EPD_BUSY) == HIGH) {
            wdt30s();
            return true;
        }
        if (millis() - lastWdt > 1000) {
            wdt120s();
            lastWdt = millis();
        }
        delay(1);
    }

#ifdef DEBUG_EPD
    printf("JD79665 busy timeout %lu ms\n", millis() - start);
#endif
    wdt30s();
    return false;
}

void jd79665::setPartialRamArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool partialMode) {
    uint16_t xe = x + w - 1;
    uint16_t ye = y + h - 1;

    epdWrite(CMD_PARTIAL_WINDOW, 9,
             x >> 8, x & 0xFF,
             xe >> 8, xe & 0xFF,
             y >> 8, y & 0xFF,
             ye >> 8, ye & 0xFF,
             partialMode ? 0x01 : 0x00);
}

void jd79665::powerOn() {
    if (!powerIsOn) {
        epdWrite(CMD_POWER_ON, 0);
        waitReady(120000);
        powerIsOn = true;
    }
}

void jd79665::powerOff() {
    if (powerIsOn) {
        epdWrite(CMD_POWER_OFF, 1, 0x00);
        waitReady(120000);
        powerIsOn = false;
    }
}

void jd79665::epdSetup() {
#ifdef DEBUG_EPD
    printf("Starting JD79665 EPD Setup\n");
#endif
    digitalWrite(EPD_BS, LOW);

    digitalWrite(EPD_RST, HIGH);
    delay(20);
    digitalWrite(EPD_RST, LOW);
    delay(2);
    digitalWrite(EPD_RST, HIGH);
    delay(20);
    waitReady(120000);
    delay(30);
    powerIsOn = false;

    epdWrite(0xAA, 6, 0x49, 0x55, 0x20, 0x08, 0x09, 0x18);
    epdWrite(0x01, 1, 0x3F);
    epdWrite(CMD_PANEL_SETTING, 2, 0x4B, 0x69);
    epdWrite(0x05, 4, 0x40, 0x1F, 0x1F, 0x2C);
    epdWrite(0x08, 4, 0x6F, 0x1F, 0x1F, 0x22);
    epdWrite(CMD_BOOSTER_SOFT_START, 4, 0x6F, 0x1F, 0x14, 0x14);
    epdWrite(0x03, 4, 0x00, 0x54, 0x00, 0x44);
    epdWrite(CMD_TCON_SETTING, 2, 0x02, 0x00);
    epdWrite(CMD_PLL_CONTROL, 1, 0x08);
    epdWrite(CMD_VCOM_INTERVAL, 1, 0x3F);
    uint16_t physicalYRes = this->effectiveYRes;
    if (tag.solumType == STYPE_SIZE_75_JD79665_BWRY_NEW) {
        physicalYRes += JD79665_NEW_GATE_GAP;
    }
    epdWrite(CMD_RESOLUTION_SETTING, 4,
             this->effectiveXRes >> 8, this->effectiveXRes & 0xFF,
             physicalYRes >> 8, physicalYRes & 0xFF);
    epdWrite(CMD_GSST_SETTING, 4, 0x10, 0x00, 0x20, 0x00);
    epdWrite(0xE3, 1, 0x2F);
    epdWrite(0x84, 1, 0x01);
}

void jd79665::epdEnterSleep() {
    powerOff();
    epdWrite(CMD_DEEP_SLEEP, 1, 0xA5);
    delay(100);
}

void jd79665::epdWriteDisplayData() {
    if (tag.solumType == STYPE_SIZE_75_JD79665_BWRY_NEW) {
        epdWriteDisplayDataWithGateGap();
        return;
    }

    uint16_t byteWidth = (this->effectiveXRes + 7) / 8;
    uint8_t *drawline_b = (uint8_t *)calloc(byteWidth, 1);
    uint8_t *drawline_r = (uint8_t *)calloc(byteWidth, 1);
    uint8_t *drawline_y = (uint8_t *)calloc(byteWidth, 1);
    uint8_t *buf = (uint8_t *)calloc(this->effectiveXRes / 4, 1);

    if (!drawline_b || !drawline_r || !drawline_y || !buf) {
        if (drawline_b) free(drawline_b);
        if (drawline_r) free(drawline_r);
        if (drawline_y) free(drawline_y);
        if (buf) free(buf);
        return;
    }

    for (uint16_t curY = 0; curY < this->effectiveYRes; curY++) {
        wdt60s();
        memset(drawline_b, 0, byteWidth);
        memset(drawline_r, 0, byteWidth);
        memset(drawline_y, 0, byteWidth);
        memset(buf, 0, this->effectiveXRes / 4);

        uint16_t sourceY = this->epdMirrorV ? (this->effectiveYRes - curY - 1) : curY;
        drawItem::renderDrawLine(drawline_b, sourceY, 0);
        drawItem::renderDrawLine(drawline_r, sourceY, 1);
        drawItem::renderDrawLine(drawline_y, sourceY, 2);
        if (this->epdMirrorH) {
            drawItem::reverseBytes(drawline_b, byteWidth);
            drawItem::reverseBytes(drawline_r, byteWidth);
            drawItem::reverseBytes(drawline_y, byteWidth);
        }

        for (uint16_t x = 0; x < this->effectiveXRes;) {
            uint8_t out = 0;
            for (uint8_t shift = 0; shift < 4; shift++) {
                uint16_t curByte = x / 8;
                uint8_t curMask = 1 << (7 - (x % 8));

                out <<= 2;
                if (drawline_r[curByte] & curMask) {
                    out |= 0x03;
                } else if (drawline_y[curByte] & curMask) {
                    out |= 0x02;
                } else if (drawline_b[curByte] & curMask) {
                    out |= 0x00;
                } else {
                    out |= 0x01;
                }
                x++;
            }
            buf[(x / 4) - 1] = out;
        }

        setPartialRamArea(0, curY, this->effectiveXRes, 1, true);
        epd_cmd(CMD_DATA_START);
        markData();
        epdSelect();
        epdSPIAsyncWrite(buf, this->effectiveXRes / 4);
        epdSPIWait();
        epdDeselect();
    }

    epdSPIWait();
    drawItem::flushDrawItems();
    free(drawline_b);
    free(drawline_r);
    free(drawline_y);
    free(buf);
}

void jd79665::epdWriteDisplayDataWithGateGap() {
    uint16_t byteWidth = (this->effectiveXRes + 7) / 8;
    uint16_t packedWidth = (this->effectiveXRes + 3) / 4;
    uint16_t gapStartY = this->effectiveYRes / 2;
    uint8_t *drawline_b = (uint8_t *)calloc(byteWidth, 1);
    uint8_t *drawline_r = (uint8_t *)calloc(byteWidth, 1);
    uint8_t *drawline_y = (uint8_t *)calloc(byteWidth, 1);
    uint8_t *buf = (uint8_t *)calloc(packedWidth, 1);

    if (!drawline_b || !drawline_r || !drawline_y || !buf) {
        if (drawline_b) free(drawline_b);
        if (drawline_r) free(drawline_r);
        if (drawline_y) free(drawline_y);
        if (buf) free(buf);
        return;
    }

    for (uint16_t curY = 0; curY < this->effectiveYRes; curY++) {
        wdt60s();
        memset(drawline_b, 0, byteWidth);
        memset(drawline_r, 0, byteWidth);
        memset(drawline_y, 0, byteWidth);
        memset(buf, 0, packedWidth);

        uint16_t sourceY = this->epdMirrorV ? (this->effectiveYRes - curY - 1) : curY;
        drawItem::renderDrawLine(drawline_b, sourceY, 0);
        drawItem::renderDrawLine(drawline_r, sourceY, 1);
        drawItem::renderDrawLine(drawline_y, sourceY, 2);
        if (this->epdMirrorH) {
            drawItem::reverseBytes(drawline_b, byteWidth);
            drawItem::reverseBytes(drawline_r, byteWidth);
            drawItem::reverseBytes(drawline_y, byteWidth);
        }

        for (uint16_t x = 0; x < this->effectiveXRes;) {
            uint8_t out = 0;
            for (uint8_t shift = 0; shift < 4; shift++) {
                uint16_t curByte = x / 8;
                uint8_t curMask = 1 << (7 - (x % 8));

                out <<= 2;
                if (drawline_r[curByte] & curMask) {
                    out |= 0x03;
                } else if (drawline_y[curByte] & curMask) {
                    out |= 0x02;
                } else if (drawline_b[curByte] & curMask) {
                    out |= 0x00;
                } else {
                    out |= 0x01;
                }
                x++;
            }
            buf[(x / 4) - 1] = out;
        }

        uint16_t physicalY = curY;
        if (curY >= gapStartY) {
            physicalY += JD79665_NEW_GATE_GAP;
        }

        setPartialRamArea(0, physicalY, this->effectiveXRes, 1, true);
        epd_cmd(CMD_DATA_START);
        markData();
        epdSelect();
        epdSPIAsyncWrite(buf, packedWidth);
        epdSPIWait();
        epdDeselect();
    }

    drawItem::flushDrawItems();
    free(drawline_b);
    free(drawline_r);
    free(drawline_y);
    free(buf);
}

void jd79665::selectLUT(uint8_t lut) {
    lut += 1;
    wdt120s();
}

void jd79665::draw() {
    drawNoWait();
    epdWaitRdy();
    getVoltage();
}

void jd79665::drawNoWait() {
    powerOn();
    epdWriteDisplayData();
    uint16_t physicalYRes = this->effectiveYRes;
    if (tag.solumType == STYPE_SIZE_75_JD79665_BWRY_NEW) {
        physicalYRes += JD79665_NEW_GATE_GAP;
    }
    setPartialRamArea(0, 0, this->effectiveXRes, physicalYRes, true);
    epdWrite(CMD_DISPLAY_REFRESH, 1, 0x00);
    delay(1);
}

void jd79665::epdWaitRdy() {
    waitReady(120000);
    powerOff();
}
