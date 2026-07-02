// HE583A04A1.h
#ifndef HE583A04A1_H
#define HE583A04A1_H

#include "EPD.h"

class HE583A04A1 : public EPD {
public:
  HE583A04A1();
  ~HE583A04A1();
  void init() override;
  void autoSequence() override;
  void writeToRAM(String rxValue, unsigned int *dataIndex, unsigned int dataLength) override;
};

#endif