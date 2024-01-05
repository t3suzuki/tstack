import subprocess, os
import textwrap

ABT_PATH = "/home/tomoya-s/work/github/ppopp21-preemption-artifact/argobots/install"
MYLIB_PATH = "/home/tomoya-s/work/pthabt/newlib"

def run(mode, n_core, arg):
    subprocess.run("sudo chcpu -e 0-{}".format(n_core-1).split())
    subprocess.run("sudo chcpu -d {}-39".format(n_core).split())
    
    my_env = os.environ.copy()
    drive_ids = [
        "0000:0d:00.0",
        "0000:0e:00.0",
        "0000:0f:00.0",
        "0000:10:00.0",
    ]
    my_env["HOOKED_FILENAME"] = "/tmp/hoge"
    my_env["MYFS_SUPERBLOCK_PATH"] = "/root/myfs_superblock"
    my_env["DRIVE_IDS"] = "_".join(drive_ids)
    #my_env["LIBDEBUG"] = MYLIB_PATH + "/debug.so"
    if mode == "abt":
        mylib_build_cmd = "make -C {} ABT_PATH={} N_CORE={} ND={} USE_PREEMPT=0".format(MYLIB_PATH, ABT_PATH, n_core, len(drive_ids))
        process = subprocess.run(mylib_build_cmd.split())
        my_env["LD_PRELOAD"] = MYLIB_PATH + "/mylib.so"
        my_env["LD_LIBRARY_PATH"] = ABT_PATH + "/lib:/home/tomoya-s/work/spdk/dpdk/build/lib"
        my_env["ABT_PREEMPTION_INTERVAL_USEC"] = "10000000"
    elif mode == "pthpth":
        mylib_build_cmd = "make pth -C {} ABT_PATH={} N_CORE={} ND={} USE_PREEMPT=1".format(MYLIB_PATH, ABT_PATH, n_core, len(drive_ids))
        process = subprocess.run(mylib_build_cmd.split())
        
        my_env["LD_PRELOAD"] = MYLIB_PATH + "/pthpth.so"
    elif mode == "io_uring":
        mylib_build_cmd = "make -C {} ABT_PATH={} N_CORE={} USE_PREEMPT=0 USE_IO_URING=1".format(MYLIB_PATH, ABT_PATH, n_core)
        process = subprocess.run(mylib_build_cmd.split())
        my_env["LD_PRELOAD"] = MYLIB_PATH + "/mylib.so"

    cmd = "./a.out {}".format(arg)
    print(cmd)
    res = subprocess.run(cmd.split(), env=my_env, capture_output=False)
    #res = subprocess.run(cmd.split(), env=my_env, capture_output=True)
    #print("captured stdout: {}".format(res.stdout.decode()))
    #print("captured stderr: {}".format(res.stderr.decode()))


#run("native", 1, 1, 0)
run("abt", 1, 3)

