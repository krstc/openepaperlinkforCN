#include <Arduino.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drawing.h"
#include "hal.h"
#include "settings.h"
#include "wdt.h"

#include "uc8253.h"

#define CMD_POWER_OFF 0x02
#define CMD_POWER_ON 0x04
#define CMD_DEEP_SLEEP 0x07
#define CMD_DISPLAY_START_TRANSMISSION_DTM1 0x10
#define CMD_DISPLAY_REFRESH 0x12
#define CMD_DISPLAY_START_TRANSMISSION_DTM2 0x13

static bool uc8253WaitReady(uint32_t timeout) {
    uint32_t start = millis();
    uint32_t lastWdt = start;

    wdt30s();

    while (millis() - start < timeout) {
        if (digitalRead(EPD_BUSY)) {
            wdt30s();
            return true;
        }
        if (millis() - lastWdt > 1000) {
            wdt30s();
            lastWdt = millis();
        }
        delay(10);
    }

#ifdef DEBUG_EPD
    printf("UC8253 busy timeout %lu ms\n", millis() - start);
#endif
    wdt30s();
    return false;
}

void uc8253::epdEnterSleep() {
    epdWrite(CMD_POWER_OFF, 0);
    uc8253WaitReady(120000);
    delay(100);
    epdWrite(CMD_DEEP_SLEEP, 1, 0xA5);
}

void uc8253::epdSetup() {
#ifdef DEBUG_EPD
    printf("Starting UC8253 EPD Setup\n");
#endif
    digitalWrite(EPD_BS, LOW);

    digitalWrite(EPD_RST, LOW);
    delay(10);
    digitalWrite(EPD_RST, HIGH);
    delay(10);

    epdWrite(CMD_POWER_ON, 0);
    uc8253WaitReady(120000);
}

void uc8253::selectLUT(uint8_t lut) {
    lut += 1;
    wdt120s();
}

void uc8253::writeBlankPlane(uint8_t cmd, uint8_t value) {
    uint16_t byteWidth = (this->effectiveXRes + 7) / 8;
    uint8_t *buf = (uint8_t *)calloc(byteWidth, 1);
    if (!buf) return;
    memset(buf, value, byteWidth);

    epd_cmd(cmd);
    markData();
    epdSelect();

    for (uint16_t y = 0; y < this->effectiveYRes; y++) {
        wdt60s();
        epdSPIAsyncWrite(buf, byteWidth);
        epdSPIWait();
    }

    epdDeselect();
    free(buf);
}

void uc8253::writePlane(uint8_t cmd, uint8_t color, bool invert) {
    uint16_t byteWidth = (this->effectiveXRes + 7) / 8;
    uint8_t *buf[2] = {0, 0};

    epd_cmd(cmd);
    markData();
    epdSelect();

    for (uint16_t curY = 0; curY < this->effectiveYRes; curY += 2) {
        wdt60s();

        uint16_t y0 = this->epdMirrorV ? (this->effectiveYRes - curY - 1) : curY;
        buf[0] = (uint8_t *)calloc(byteWidth, 1);
        if (!buf[0]) break;
        drawItem::renderDrawLine(buf[0], y0, color);
        if (this->epdMirrorH) drawItem::reverseBytes(buf[0], byteWidth);
        if (invert) {
            for (uint16_t i = 0; i < byteWidth; i++) buf[0][i] ^= 0xFF;
        }

        if (buf[1]) {
            epdSPIWait();
            free(buf[1]);
            buf[1] = 0;
        }
        epdSPIAsyncWrite(buf[0], byteWidth);

        buf[1] = (uint8_t *)calloc(byteWidth, 1);
        if (!buf[1]) {
            epdSPIWait();
            free(buf[0]);
            buf[0] = 0;
            break;
        }
        if ((curY + 1) < this->effectiveYRes) {
            uint16_t y1 = this->epdMirrorV ? (this->effectiveYRes - curY - 2) : (curY + 1);
            drawItem::renderDrawLine(buf[1], y1, color);
            if (this->epdMirrorH) drawItem::reverseBytes(buf[1], byteWidth);
        }
        if (invert) {
            for (uint16_t i = 0; i < byteWidth; i++) buf[1][i] ^= 0xFF;
        }

        epdSPIWait();
        free(buf[0]);
        buf[0] = 0;
        epdSPIAsyncWrite(buf[1], byteWidth);
    }

    epdSPIWait();
    epdDeselect();
    if (buf[0]) free(buf[0]);
    if (buf[1]) free(buf[1]);
}

void uc8253::epdWriteDisplayData() {
#ifdef DEBUG_EPD
    printf("UC8253: Render Start\n");
#endif

    if (tag.thirdColor) {
        writePlane(CMD_DISPLAY_START_TRANSMISSION_DTM1, 0, true);
        writePlane(CMD_DISPLAY_START_TRANSMISSION_DTM2, 1, true);
    } else {
        writePlane(CMD_DISPLAY_START_TRANSMISSION_DTM1, 0, true);
        writeBlankPlane(CMD_DISPLAY_START_TRANSMISSION_DTM2, 0x00);
    }

    drawItem::flushDrawItems();
}

void uc8253::draw() {
    drawNoWait();
    epdWaitRdy();
    getVoltage();
}

void uc8253::drawNoWait() {
    epdWriteDisplayData();

    epdWrite(CMD_DISPLAY_REFRESH, 0);
    delay(1);
}

void uc8253::epdWaitRdy() {
    uc8253WaitReady(120000);
}
