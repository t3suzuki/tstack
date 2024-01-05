#pragma once

#include "syncmem.h"

class SyncMemNop : public SyncMem {
 private:
  int fd;
  char buf[N_TH][UNIT_SIZE];
 public:
  SyncMemNop() {
  };
  int read(int i_th, uint64_t offset) {
    sched_yield();
    return 0;
  };
};
