
#pragma once


#include "tnvme.h"
#include "syncmem.h"

class SyncMemTNVMe : public SyncMem {
 private:
  int fd;
  char buf[N_TH][UNIT_SIZE];
 public:
  SyncMemTNVMe() {
    nvme_init();
  };
  int read(int i_th, uint64_t offset) {
    uint64_t lba = offset;
    int i_core = 0;
    int rid = nvme_read_req(lba/512, 1, i_core, UNIT_SIZE, buf[i_th]);
    while (1) {
      sched_yield();
      if (nvme_check(rid))
	break;
    }
    return 0;
  };
};
