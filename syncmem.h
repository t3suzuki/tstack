#pragma once

class SyncMem {
 public:
  virtual int read(int i_th, uint64_t offset) = 0;
};
