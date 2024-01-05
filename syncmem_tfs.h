
#pragma once


#include "tnvme.h"
#include "myfs.h"
#include "syncmem.h"

class SyncMemTFS : public SyncMem {
 private:
  int fd;
  char buf[N_TH][UNIT_SIZE];
 public:
  SyncMemTFS() {
    myfs_mount("/root/myfs_superblock");
    nvme_init();
    fd = myfs_open("/tmp/hoge");
  };
  int read(int i_th, uint64_t offset) {
    int64_t lba = myfs_get_lba(fd, offset/512*512, 0);
    int i_core = 0;
    int rid = nvme_read_req(lba, 1, i_core, UNIT_SIZE, buf[i_th]);
    while (1) {
      sched_yield();
      if (nvme_check(rid))
	break;
    }
    return 0;
  };
};
