#CXX = clang++
CXX = g++

CFLAGS = -g
LIBS =

SPDK_DIR = /home/tomoya-s/work/spdk/build
DPDK_DIR = /home/tomoya-s/work/spdk/dpdk/build
CFLAGS += -I $(SPDK_DIR)/include
CFLAGS += -DN_CORE=1
CFLAGS += -DN_SSD=4
CFLAGS += -DN_CORO=4096

CFLAGS += -DUNIT_SIZE=64
CFLAGS += -DN_TH=256


SPDK_LIBS = -Wl,--whole-archive
SPDK_LIBS += -L $(SPDK_DIR)/lib -lspdk_sock_posix -lspdk_nvme -lspdk_sock -lspdk_trace -lspdk_rpc -lspdk_jsonrpc -lspdk_json -lspdk_vfio_user -lspdk_vmd -lspdk_util -lspdk_log -lspdk_env_dpdk
SPDK_LIBS += -Wl,--no-whole-archive
SPDK_LIBS += -L $(DPDK_DIR)/lib -lrte_eal -lrte_kvargs -lrte_pci -lrte_bus_pci -lrte_mempool -lrte_ring -lrte_telemetry
SPDK_LIBS += -luuid -lssl -lcrypto -lnuma

LIBS += $(SPDK_LIBS)
LIBS += -ltbb
LIBS += -lpthread

all2:
	$(CXX) -march=native -O3 $(CFLAGS) pth_perf.c tnvme.cc myfs.c -DUSE_PTHPTH=1 $(LIBS)
dummy:
	$(CXX) -DUSE_PTHPTH=1 -DN_CORE=1 -DN_SSD=7 -march=native -O3 $(CFLAGS) dummy_write.c -o dummy_write tnvme.cc myfs.c
	$(CXX) -DUSE_PTHPTH=1 -DN_CORE=1 -DN_SSD=7 -march=native -O3 $(CFLAGS) dummy_read.c -o dummy_read tnvme.cc myfs.c

all:
	$(CXX) -march=native -O3 $(CFLAGS) my_perf.c zipf.c tnvme.cc myfs.c $(LIBS)
clean:
	rm a.out *~

prep:
	echo 2048 | sudo tee -a /proc/sys/vm/nr_hugepages


callback:
	$(CXX) -march=native -O3 $(CFLAGS) callback.c $(LIBS) -o callback
