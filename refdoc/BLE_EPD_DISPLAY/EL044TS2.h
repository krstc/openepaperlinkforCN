// EL044TS2.h
#ifndef EL044TS2_H
#define EL044TS2_H

#include "EPD.h"

class EL044TS2 : public EPD {
public:
  EL044TS2();
  ~EL044TS2();
  void init() override;
  void writeToRAM(String rxValue, unsigned int *dataIndex, unsigned int dataLength) override;
};

#endif