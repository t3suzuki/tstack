
#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "syncmem.h"

class SyncMemPread : public SyncMem {
 private:
  int fd;
  char buf[N_TH][UNIT_SIZE];
 public:
  SyncMemPread() {
    fd = open("/tmp/hoge", O_RDONLY);
  };
  int read(int i_th, uint64_t offset) {
    //printf("%s %d\n", __func__, __LINE__);
    pread(fd, buf[i_th], UNIT_SIZE, offset / 512 * 512);
    //sched_yield();
    return ((int*)buf)[0];
  };
};
