#include <stdint.h>
#include "tnvme.h"
#include "myfs.h"

#define ADDR_RANGE (1024ULL*1024*1024/2)

int
main()
{
  nvme_init();

  uint64_t i;
  myfs_mount("/root/myfs_superblock");
  int fd = myfs_open("/tmp/hoge");
  
  for (i=0; i<ADDR_RANGE; i++) {
    myfs_get_lba(fd, i, 1);
  }
  myfs_close();
  myfs_umount();
}
