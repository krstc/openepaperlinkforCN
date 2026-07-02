#ifndef _EPD_JD79665_H_
#define _EPD_JD79665_H_

class jd79665 : public epdInterface {
   public:
    void epdSetup();
    void epdEnterSleep();
    void draw();
    void drawNoWait();
    void epdWaitRdy();
    void selectLUT(uint8_t lut);

   protected:
    void epdWriteDisplayData();

   private:
    bool powerIsOn = false;
    bool waitReady(uint32_t timeout);
    void powerOn();
    void powerOff();
    void setPartialRamArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool partialMode);
};

#endif
