#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <immintrin.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include "syncmem.h"
#include "syncmem_pread.h"
#include "syncmem_nop.h"
#include "syncmem_spdk.h"
#include "syncmem_tnvme.h"
#include "syncmem_tfs.h"


#define _GNU_SOURCE
#include <sched.h>


#define ADDR_RANGE (1024ULL*1024*1024/2)
#define TIME_SEC (10)

#define THETA (0.3)


uint64_t g_yield_cnt = 0;
static uint64_t g_iter[N_TH];
static uint64_t g_sum[N_TH];

volatile bool begin = false;
volatile bool quit = false;

typedef struct {
  uint32_t i_th;
  SyncMem *syncmem;
} arg_t;

void
worker(void *a)
{
  arg_t *arg = (arg_t *)a;
  //struct zipf * zipf = zipf_create(ADDR_RANGE, THETA, arg->i_);

  while (1) {
    if (begin)
      break;
    //_mm_pause();
    sched_yield();
  }

  //printf("%s %d\n", __func__, __LINE__);
  uint64_t iter = 0;
  uint32_t seed = arg->i_th;
  uint64_t sum = 0;
  while (quit == false) {
    uint64_t v = iter++;

    //addr = zipf_generate(zipf);
    uint64_t addr = (uint64_t)rand_r(&seed) % ADDR_RANGE;

    sum += arg->syncmem->read(arg->i_th, addr);
    //printf("v = %d\n", v);
  }

  g_iter[arg->i_th] = iter;
  g_sum[arg->i_th] = sum;
  
}




void run_test(SyncMem *syncmem)
{
  printf("Start test...\n");
  
  pthread_t pth[N_TH];
  arg_t arg[N_TH];
  for (auto i_th=0; i_th<N_TH; i_th++) {
    arg[i_th].i_th = i_th;
    arg[i_th].syncmem = syncmem;
    pthread_create(&pth[i_th], NULL, (void *(*)(void*))worker, &arg[i_th]);
  }
  auto start = std::chrono::steady_clock::now();
  begin = true;
  for (auto i=1; i<=TIME_SEC; i++) {
    sleep(1);
    printf("Elapsed %d/%d\n", i, TIME_SEC);
  }
  quit = true;
  auto end = std::chrono::steady_clock::now();

  for (auto i_th=0; i_th<N_TH; i_th++) {
    pthread_join(pth[i_th], NULL);
  }

  uint64_t sum_iter = 0;
  uint64_t sum_sum = 0;
  for (auto i_th=0; i_th<N_TH; i_th++) {
    sum_iter += g_iter[i_th];
    sum_sum += g_sum[i_th];
  }
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
  std::cout << "elapsed time: " << elapsed.count() << "ms\n";
  std::cout << sum_iter / 1000.0 / elapsed.count() << " M IOPS" << std::endl;

  std::cout << sum_iter << std::endl;
  std::cout << g_yield_cnt << std::endl;
  sleep(1);
  
  struct rusage ru;
  getrusage(RUSAGE_SELF, &ru);
  printf("Max RSS: %f MB\n", ru.ru_maxrss / 1024.0);
}


int
main(int argc, char **argv)
{
  std::cout << "Using " << N_TH << " threads. " << std::endl;
  
  std::cout << "Running..." << std::endl;

  int mode = 0;
  if (argc > 1) {
    mode = atoi(argv[1]);
  }
  SyncMem *syncmem;
  switch (mode) {
  case 0:
    syncmem = new SyncMemPread();
    break;
  case 1:
    //syncmem = new SyncMemSPDK();
    break;
  case 2:
    syncmem = new SyncMemTNVMe();
    break;
  case 3:
    syncmem = new SyncMemNop();
    break;
  case 4:
    syncmem = new SyncMemTFS();
    break;
  }
  run_test(syncmem);
  std::cout << "Done!" << std::endl;
  exit(0);
}
