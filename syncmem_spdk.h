
#pragma once

#include <vector>

#include "syncmem.h"
#include "spdk/nvme.h"


#define N_QP (N_CORE + 1)

typedef struct {
  struct spdk_nvme_ns *ns;
  struct spdk_nvme_qpair *qpairs[N_QP];
} drive_t;
struct spdk_nvme_poll_group *group;

static inline void
memspdk_read_complete(void *arg, const struct spdk_nvme_cpl *completion)
{
  int *done_flag = (int *)arg;
  *done_flag = 1;
}

static bool
memspdk_probe_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
	      struct spdk_nvme_ctrlr_opts *opts)
{
  opts->io_queue_size = UINT16_MAX;
  return true;
}

struct spdk_nvme_qpair *
memspdk_alloc_qpair(struct spdk_nvme_ctrlr *ctrlr)
{
  struct spdk_nvme_io_qpair_opts opts;
  struct spdk_nvme_qpair *qpair;
  spdk_nvme_ctrlr_get_default_io_qpair_opts(ctrlr, &opts, sizeof(opts));
  //printf("%d\n", opts.io_queue_requests);
  opts.delay_cmd_submit = true;
  opts.create_only = true;
  qpair = spdk_nvme_ctrlr_alloc_io_qpair(ctrlr, &opts, sizeof(opts));

  int rc = spdk_nvme_poll_group_add(group, qpair);
  if (rc != 0) {
    assert(0);
  }
  spdk_nvme_ctrlr_connect_io_qpair(ctrlr, qpair);
  return qpair;
}

static void
memspdk_attach_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
		  struct spdk_nvme_ctrlr *ctrlr, const struct spdk_nvme_ctrlr_opts *opts);


static void
memspdk_init()
{
  int rc;
  struct spdk_env_opts opts;
  struct spdk_nvme_transport_id g_trid = {};
  
  spdk_env_opts_init(&opts);
  
  spdk_nvme_trid_populate_transport(&g_trid, SPDK_NVME_TRANSPORT_PCIE);
  snprintf(g_trid.subnqn, sizeof(g_trid.subnqn), "%s", SPDK_NVMF_DISCOVERY_NQN);
  
  if (spdk_env_init(&opts) < 0) {
    fprintf(stderr, "Unable to initialize SPDK env\n");
    exit(__LINE__);
  }

  printf("hoge\n");
  rc = spdk_nvme_probe(&g_trid, NULL, memspdk_probe_cb, memspdk_attach_cb, NULL);
  
  printf("Initialization complete.\n");

}


static std::vector<drive_t *> g_drives;

class SyncMemSPDK : public SyncMem {
 private:
  char *rbuf[N_TH];
  int done_flag[N_TH];
  
 public:
  SyncMemSPDK() {
    memspdk_init();
    
    for (int i=0; i<N_TH; i++) {
      rbuf[i] = (char *)spdk_zmalloc(0x1000, 0x1000, NULL, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);
    }
  };
  void register_drive(drive_t *drive) {
    g_drives.push_back(drive);
  }

  inline int read(int i_th, int offset) {
    int i_core = 0;
    uint64_t addr = offset * UNIT_SIZE;
    int ssd = addr % N_SSD;
    uint64_t lba = addr / N_SSD;
    done_flag[i_th] = 0;
    spdk_nvme_ns_cmd_read(g_drives[ssd]->ns, g_drives[ssd]->qpairs[i_core], rbuf[i_th],
			  lba,
			  1,
			  memspdk_read_complete, &done_flag[i_th], 0);
    do {
      sched_yield();
      spdk_nvme_qpair_process_completions(g_drives[ssd]->qpairs[i_core], 0);
    } while (done_flag[i_th] == 0);
    
    return ((int*)rbuf)[0];
  };
};


static void
memspdk_attach_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
		  struct spdk_nvme_ctrlr *ctrlr, const struct spdk_nvme_ctrlr_opts *opts)
{
  printf("Attached to NVMe Controller at %s\n", trid->traddr);

  struct spdk_nvme_ns *ns;
  uint32_t nsid;

  group = spdk_nvme_poll_group_create(NULL, NULL);
  
  for (nsid = spdk_nvme_ctrlr_get_first_active_ns(ctrlr);
       nsid != 0; nsid = spdk_nvme_ctrlr_get_next_active_ns(ctrlr, nsid)) {
    ns = spdk_nvme_ctrlr_get_ns(ctrlr, nsid);
    if (ns == NULL) {
      continue;
    }

    drive_t *drive = (drive_t *)calloc(1, sizeof(drive_t));
    drive->ns = ns;
    for (int i_qp=0; i_qp<N_QP; i_qp++) {
      drive->qpairs[i_qp] = memspdk_alloc_qpair(ctrlr);
    }
    ((SyncMemSPDK *)cb_ctx)->register_drive(drive);
  }
}
