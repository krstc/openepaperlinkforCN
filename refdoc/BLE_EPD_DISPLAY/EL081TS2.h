// EL081TS2.h
#ifndef EL081TS2_H
#define EL081TS2_H

#include "EPD.h"

class EL081TS2 : public EPD {
public:
  EL081TS2();
  ~EL081TS2();
  void init() override;
  void writeToRAM(String rxValue, unsigned int *dataIndex, unsigned int dataLength) override;
};

#endif