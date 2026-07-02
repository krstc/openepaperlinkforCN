#ifndef _EPD_UC8253_H_
#define _EPD_UC8253_H_

class uc8253 : public epdInterface {
   public:
    void epdSetup();
    void epdEnterSleep();
    void draw();
    void drawNoWait();
    void epdWaitRdy();
    void epdWriteDisplayData();
    void selectLUT(uint8_t lut);

   protected:
    void writeBlankPlane(uint8_t cmd, uint8_t value);
    void writePlane(uint8_t cmd, uint8_t color, bool invert);
};

#endif
