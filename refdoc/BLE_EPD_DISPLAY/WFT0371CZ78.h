// WFT0371CZ78.h
#ifndef WFT0371CZ78_H
#define WFT0371CZ78_H

#include "EPD.h"

class WFT0371CZ78 : public EPD {
public:
  WFT0371CZ78();
  ~WFT0371CZ78();
  void init() override;
  void autoSequence() override;
  void writeToRAM(String rxValue, unsigned int *dataIndex, unsigned int dataLength) override;
};

#endif