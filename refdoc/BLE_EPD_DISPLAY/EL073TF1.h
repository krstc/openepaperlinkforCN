// EL073TF1.h
#ifndef EL073TF1_H
#define EL073TF1_H

#include "EPD.h"

class EL073TF1 : public EPD {
public:
  EL073TF1();
  ~EL073TF1();
  void init() override;
  void writeToRAM(String rxValue, unsigned int *dataIndex, unsigned int dataLength) override;
};

#endif