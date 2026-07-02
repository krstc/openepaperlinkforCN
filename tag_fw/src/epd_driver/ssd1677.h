#ifndef _EPD_SSD1677_H_
#define _EPD_SSD1677_H_

class ssd1677 : public epdInterface {
   public:
    void epdSetup();
    void epdEnterSleep();
    void draw();
    void drawNoWait();
    void epdWaitRdy();
    void epdWriteDisplayData();
    void selectLUT(uint8_t lut);

   protected:
    void setWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void setRamPointer(uint16_t x, uint16_t y);
    void writePlane(uint8_t cmd, uint8_t color);
};

#endif
