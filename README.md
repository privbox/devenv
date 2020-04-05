# Artifact Evaluation submission for Privbox [USENIX ATC '22]

**Paper:** Privbox: Faster System Calls Through Sandboxed Privileged Execution 

This README is available at: https://github.com/privbox/devenv/blob/privbox/README.md

## Overview

This document accompanies the paper and describes how to set up an enviroment that can be used to run the experiments we describe in the paper.

Our work relies on 2 main components:

1. A toolchain capable of producing instrumented code
2. A VM running a modified Linux OS capabled of running privboxed code.

All of the experiments described are executed *inside* the VM running the modified Linux OS. The toolchain is required to produce binaries we run in the experiments themselves.


<div style="page-break-after: always;"></div>

## Environment setup (with Docker)

To ensure all of the commands are executed in the same way they did for us, we assume a Fedora 36 host.
To avoid the hassle of installing an OS from scratch, we provide a Docker image that can be used as the enviroment for the evaluation.
Once built, it contains all the necessary dependecies to build the artefacts and run the experiments.

```
# Clone devenv repo
git clone -b privbox https://github.com/privbox/devenv
cd devenv

# Build development env container
./devenv.build

# Attach to shell in the container
./devenv.run
```

Note: in provided Docker image, `WORKDIR` is set up automatically and points to `/workdir`

<div style="page-break-after: always;"></div>

## Environment setup (manual)
**(Skip this if using docker)**

1. Install a Fedora 36 host
2. Install dependencies:
   ```
   sudo dnf install -y \
    'dnf-command(builddep)' \
    git \
    ninja-build podman \
    libguestfs-tools \
    qemu-img \
    flex \
    bison \
    elfutils-libelf-devel \
    openssl-devel \
    glibc-static \
    automake \
    libtool \
    rsync \
    bc \
    hostname \
    sudo
    

   sudo dnf builddep -y clang
   ```
3. Set WORKDIR to path where env will be set up `export WORKDIR=${HOME}/projects/privbox`

<div style="page-break-after: always;"></div>

## Clone sources

Run the following commands to fetch all the sources required to build the artefacts:
```
cd ${WORKDIR?}
git clone -b privbox https://github.com/privbox/llvm-project
git clone -b privbox https://github.com/privbox/linux kernel
git clone -b privbox https://github.com/privbox/musl
git clone -b privbox https://github.com/privbox/devenv

# Benchmarks
git clone -b privbox https://github.com/privbox/redis
git clone https://github.com/privbox/redis redis-vanilla
git clone https://github.com/RedisLabs/memtier_benchmark
git clone -b privbox https://github.com/privbox/libevent
git clone -b privbox https://github.com/privbox/memcached
git clone -b privbox https://github.com/privbox/sqlite-bench
git clone -b privbox https://github.com/privbox/piotbench
```
<div style="page-break-after: always;"></div>

## Build prototype VM kernel / initrd / rootfs

The following builds the virtual enviroment containing our prototype. This builds all the resources we provide qemu with in order to boot the VM: kernel containing our changes, initrd, and rootfs with the filesystem containing all dependencies needed to run the benchmarks.

```
cd ${WORKDIR?}/devenv/images
make kernel-headers kernel initrd rootfs tools

# Note: sometimes, kernel build will fail, try building it without concurrency:
(cd ${WORKDIR?}/kernel; make bzImage -j1); make kernel
```

<div style="page-break-after: always;"></div>

## Build toolchain

The following builds LLVM toolchain capable of producing instrumentation required for privboxing, as well as C libraries instrumented in various ways.

*Note*: building LLVM can take a while, and at final stages, it links compiled objects into binaries, which consumes a lot of memory. By default, build is parallelized based on number of available cores, therefore, at later stages, many link jobs might be running concurrently, possible depleting all available memory on the machine. If this happens, run build step manually with limited concurrency.

Once LLVM is built, `clang` is available in `${WORKDIR?}/llvm-project/bin` directory.

```
# llvm:
cd ${WORKDIR?}/llvm-project

# Configure:
cmake \
    -S llvm \
    -B build \
    -G Ninja \
    -DLLVM_ENABLE_PROJECTS='clang;compiler-rt;lld' \
    -DLLVM_TARGETS_TO_BUILD=X86 \
    -DBUILD_SHARED_LIBS=OFF
    
# Compile:
cmake --build build

# Note: if this fails due to lack of memory, run: cmake --build build -j 1

# musl:
cd ${WORKDIR?}/musl
./build.sh
```

<div style="page-break-after: always;"></div>

## Build benchmarks

The following commands build all the binaries needed to produce the results of the paper. Feel free to adjust only to relevant subset.

```
# redis:
cd ${WORKDIR?}/redis
./build-all.sh

cd ${WORKDIR?}/redis-vanilla
make -j$(nproc)
cp src/redis-benchmark ${WORKDIR?}/devenv/bin/

cd ${WORKDIR?}/memtier_benchmark
autoreconf -ivf
./configure
make -j$(nproc)
cp memtier_benchmark ${WORKDIR?}/devenv/bin/

# Memcached:
cd ${WORKDIR?}/libevent
./autogen.sh
./build.sh
cd ${WORKDIR?}/memcached
./autogen.sh
./build.sh

# sqlite-bench:
cd ${WORKDIR?}/sqlite-bench
./build.sh

# piotbench:
cd ${WORKDIR?}/piotbench
./build-all.sh
```

<div style="page-break-after: always;"></div>

# Running virtualized env for Privbox prototype

Our scripts use QEMU to spin up a VM running the patched kernel. We provide network connectivity to the VM to enable ssh access (in cases where a serial console is not enough).
Network connectivity is based on bridge/tap interfaces and require sudo/root access to set up. Make sure `network.sh` script runs prior to execution (created interfaces are ephemeral and need to be re-created each reboot/container restart).

```
cd ${WORKDIR?}/devenv/scripts

# Network interfaces setup (needed only once per boot, creates network interfaces piot-br/piot-tap)
sudo ./network.setup

# undo with sudo ./network.sh teardown

# Start VM:
./run.sh
# Exit with ctrl-a, x

# Access possible through serial console (on window running run.sh) or through ssh root@192.168.232.10 (password: 123)

```

<div style="page-break-after: always;"></div>

# Pre-evaluation suggestions:
Below as some suggestions that can be useful in achieving a stable testing environment.

1. Turn on hyper threading (in BIOS)
2. Turn of CPU scaling:
```
sudo bash -c 'for X in $(ls -1 /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor); do echo performance > $X; done'
```

3. Adjust run.sh to create an adequate VM for your env, specifically: 
  `-smp` param, to specify number of cores for VM,
  `-m` param, to specify the VM memory size (in MiB),
 
4. Core pinning can result is less noisy results (e.g. pin QEMU - in run.sh - to specific physical cores, or, pin evaluated workloads inside the prototype VM, e.g. redis-server, to specific VM cores). Not mandatory, see taskset for specifics.

<div style="page-break-after: always;"></div>

# Running experiments:

## Entry overhead (Section 6.1 & Figure 4)

One of the first results we present is the overhead of system call with, without PTI and in Privbox.

We provide mtest utility that reports roundtrip latency of regular system call vs call from privbox.
Output of the utility is 2 lines, for example:

```
priv = 4362551204, per call 436
user = 9533344140, per call 953
```
`priv = ` line reports results for invocation from within privbox and `user =` for regular system calls. `per call DDD` signifies the number of cycles a round trip took on average.

To start the experiment, start the VM:
```
cd ${WORKDIR?}/devenv/scripts

# To run VM with PTI
./run.sh 

# To run VM without PTI
./run-nopti.sh
```

Once inside the VM, run:
```
/proj/images/dist/tools/bin/mtest
```

<div style="page-break-after: always;"></div>

## Piotbench (Section 6.3 & Figure 5)

In this experiment we benchmark a client-server program we build specifically to explore privbox under different levels of compute/IO loads. Our experiment evaluates regular system-call based execution (both with and without PTI) and Privbox.

To perform the measurements themselves, run piotbench runner script (inside VM)
Note: execution a signle setup with default parameters is quite lengthy (about 1.5 hours), consult with params at the top of the runner script to reduce duration (`DURATION`)/number of iterations (`CLIENT_ITERS`) / number of tested computation length values (`COMP_VALUES`) / number of tested IO size values (`IO_SIZE_VALUES`).

* Results for syscall (no PTI):
```
cd ${WORKDIR?}/devenv/scripts
./run-nopti.sh

# Run the following inside VM:

# Perform measurements:
export KERNCALL=0
/proj/bin/piotbench-noinstr/runner.sh test

# Generate report:
/proj/bin/piotbench-noinstr/runner.sh report
```

* Results for syscall (with PTI):
```
cd ${WORKDIR?}/devenv/scripts
./run.sh

# Run the following inside VM:

# Perform measurements:
export KERNCALL=0
/proj/bin/piotbench-noinstr/runner.sh test

# Generate report:
/proj/bin/piotbench-noinstr/runner.sh report
```
* Results for privbox:
```
cd ${WORKDIR?}/devenv/scripts
./run.sh

# Run the following inside VM:

# Perform measurements:
export KERNCALL=1
/proj/bin/piotbench-fullinstr/runner.sh test

# Generate report:
/proj/bin/piotbench-fullinstr/runner.sh report
```

<div style="page-break-after: always;"></div>

## redis-server+ redis-benchmark (Section 6.3-redis & Figure 6)

In this experiment we run `redis-server` under different levels of instrumentation, with and without privbox, and evalue performance using `redis-benchmark` load generator.

To start the virtualized environment, run:

```
cd ${WORKDIR?}/devenv/scripts
./run.sh
```

Note: for this benchmark you'll probably want to attach a second shell to run several command in parallel on the VM. Use ssh: `ssh root@192.168.232.10` (password: 123)

There are 6 different setups we evaluate, first, run a `redis-server` instance with a desired config inside the VM:

```
# fullinstr-nokerncall:
/proj/bin/redis-fullinstr/redis-server --save "" --appendonly no --io-threads 1 --kerncall no

# fullinstr-kerncall:
/proj/bin/redis-fullinstr/redis-server --save "" --appendonly no --io-threads 1 --kerncall yes

# brinstr-nokerncall:
/proj/bin/redis-instr/redis-server --save "" --appendonly no --io-threads 1 --kerncall no

# brinstr-kerncall:
/proj/bin/redis-instr/redis-server --save "" --appendonly no --io-threads 1 --kerncall yes

# noinstr-nokerncall:
/proj/bin/redis-noinstr/redis-server --save "" --appendonly no --io-threads 1 --kerncall no

# noinstr-kerncall:
/proj/bin/redis-noinstr/redis-server --save "" --appendonly no --io-threads 1 --kerncall yes
```

Then, in a different shell, run `redis-benchmark` to generate load:
```
# Perform measurements:
for I in $(seq 0 10); do
    echo $I
    /proj/bin/redis-benchmark \
    	--csv \
        -t ping,get,set,incr \
        --threads 2 \
        -n 1000000 \
        -c 50 \
        | tee /tmp/res.$I.csv
done

# Generate report:
for TEST in PING_INLINE PING_MBULK GET SET INCR
do
    for I in $(seq 1 10)
    do
        grep $TEST /tmp/res.$I.csv | tr '"' ' ' | awk '{print $NF}'
    done | tee /tmp/res.$TEST
    awk '{ sum += $1; count += 1; print $1 } END { print sum / count }' /tmp/res.$TEST > /tmp/res.$TEST.avg
done
for TEST in PING_INLINE PING_MBULK GET SET INCR
do
    echo -n "$TEST "
    cat /tmp/res.$TEST.avg | xargs echo
done
```

Report includes measurement (in requests per second) of each iteration (as well as average) for each test type.

<div style="page-break-after: always;"></div>

## redis-server + memtier_benchmark (Section 6.3 & Figure 7)

In this experiment we run `redis-server` under different levels of instrumentation, with and without privbox, and evaluate performance using `memtier_benchmark` load generator.

To start the virtualized environment, run:

```
cd ${WORKDIR?}/devenv/scripts
./run.sh
```

Note: for this benchmark you'll probably want to attach a second shell to run several command in parallel on the VM. Use ssh: `ssh root@192.168.232.10` (password: 123)

There are 6 different setups we evaluate, first, run a `redis-server` instance with a desired config inside the VM:

```
# fullinstr-nokerncall:
/proj/bin/redis-fullinstr/redis-server --save "" --appendonly no --io-threads 1 --kerncall no

# fullinstr-kerncall:
/proj/bin/redis-fullinstr/redis-server --save "" --appendonly no --io-threads 1 --kerncall yes

# brinstr-nokerncall:
/proj/bin/redis-instr/redis-server --save "" --appendonly no --io-threads 1 --kerncall no

# brinstr-kerncall:
/proj/bin/redis-instr/redis-server --save "" --appendonly no --io-threads 1 --kerncall yes

# noinstr-nokerncall:
/proj/bin/redis-noinstr/redis-server --save "" --appendonly no --io-threads 1 --kerncall no

# noinstr-kerncall:
/proj/bin/redis-noinstr/redis-server --save "" --appendonly no --io-threads 1 --kerncall yes
```

Then, in a different shell, run `memtier_benchmark` to generate load:
```
# Perform measurements:
for I in $(seq 0 10); do
    /proj/bin/memtier_benchmark \
        --hide-histogram \
        -n 100000 \
        --json-out-file=/tmp/res.$I.json
done

# Generate report:
for I in $(seq 1 10); do
    jq '."ALL STATS".Totals."Ops/sec" ' /tmp/res.$I.json
done | tee > /tmp/res.all

awk '{ sum += $1; count += 1; print $1 } END { print sum / count }' /tmp/res.all
```

Generated report shows requests/second of each iteration, as well as the average (last line)

Repeat measurement gathering with different server parameters if needed.

<div style="page-break-after: always;"></div>

## memcached + memtier_benchmark (Section 6.3-memcached & Figure 8)

In this experiment we run `memcached` under different levels of instrumentation, with and without privbox, and evaluate performance using `memtier_benchmark` load generator.

To start the virtualized environment, run:

```
cd ${WORKDIR?}/devenv/scripts
./run.sh
```

Note: for this benchmark you'll probably want to attach a second shell to run several command in parallel on the VM. Use ssh: `ssh root@192.168.232.10` (password: 123)

There are 6 different setups we evaluate, first, run a `memcached` instance with a desired config inside the VM:

```
# fullinstr-nokerncall:
/proj/bin/memcached-fullinstr/memcached -u root --threads=1

# fullinstr-kerncall:
/proj/bin/memcached-fullinstr/memcached -u root -K --threads=1

# brinstr-nokerncall:
/proj/bin/memcached-instr/memcached -u root --threads=1

# brinstr-kerncall:
/proj/bin/memcached-instr/memcached -u root -K --threads=1

# noinstr-nokerncall:
/proj/bin/memcached-noinstr/memcached -u root --threads=1

# noinstr-kerncall:
/proj/bin/memcached-noinstr/memcached -u root -K --threads=1
```

Then, in a different shell, run `memtier_benchmark` to generate load:
```
# Perform measurements:
for I in $(seq 0 10); do
    taskset -c 10,11,12,13 nice --19 /proj/bin/memtier_benchmark \
        -P memcache_binary\
        -p 11211 \
        --hide-histogram \
        -n 100000 \
        --json-out-file=/tmp/res.$I.json
done

# Generate report:
for I in $(seq 1 10); do
    jq '."ALL STATS".Totals."Ops/sec" ' /tmp/res.$I.json
done | tee > /tmp/res.all
awk '{ sum += $1; count += 1; print $1 } END { print sum / count }' /tmp/res.all
```

Generated report shows requests/second of each iteration, as well as the average (last line)

Repeat measurement gathering with different server parameters if needed.

<div style="page-break-after: always;"></div>

## sqlite-bench (Section 6.3-sqlite & Figure 9)

In this experiment we run `sqlite-bench` utility under different levels of instrumentation, with and without privbox and evaluate perfomance based on the reported latency of different operations.

To start the virtualized environment, run:
```
cd ${WORKDIR?}/devenv/scripts
./run.sh
```

Then, run the measurements inside the VM:

```
rm -rf /tmp/sqlite
mkdir -p /tmp/sqlite
for TEST in fillseq fillseqsync fillseqbatch fillrandom fillrandsync \
    fillrandbatch overwrite overwritebatch readrandom readseq \
    fillrand100K fillseq100K readrand100K;
do
    for I in $(seq 0 10); do
        for KERNCALL in 0 1;
        do
            for INSTR in fullinstr instr noinstr;
            do
                echo $TEST: $(( $I + 1 ))/11, KERNCALL=$KERNCALL INSTR=$INSTR
                /proj/bin/sqlite-bench-$INSTR/sqlite-bench \
                    --num=1 --kerncall=$KERNCALL --benchmarks=$TEST 2>&1 | grep micro | \
                     tee /tmp/sqlite/res.$INSTR.$KERNCALL.$TEST.$I
            done
        done
    done
done

# Report
for TEST in fillseq fillseqsync fillseqbatch fillrandom fillrandsync \
    fillrandbatch overwrite overwritebatch readrandom readseq \
    fillrand100K fillseq100K readrand100K;
do
    echo -n $TEST,;
    for I in $(seq 1 10);
    do
        grep -oE "\b$TEST\b.*$" /tmp/res.$I | head -1 | awk '{print $3}'
    done | tr '\n' ,
    echo
done
```

Report produces a single line for each test detailing the duration of each test (in micros/op)

<div style="page-break-after: always;"></div>

## SMAP overhead (Section 5-Expected overhead & Figure 3)

In this experiment we measure the effects of SMAP on list traversal inside kernel. We're using a kernel module compiled into our prototype's kernel to run the benchmark.
The experiment is to be ran twice, once with SMAP and once without.

* To run the VM with SMAP, run:
```
cd ${WORKDIR?}/devenv/scripts
./run.sh
```
* To run the VM without SMAP, run:
```
cd ${WORKDIR?}/devenv/scripts
./run.sh
```
* To run the VM with SMAP, run:
```
cd ${WORKDIR?}/devenv/scripts
./run-smap.sh
```

Once inside the VM env, run the following inside the VM to measure traversal times

```
KB=$((2<<10))
MB=$((2<<20))
run_series () {
        echo $1 > /sys/module/smapbench/parameters/list_len
        echo $2 > /sys/module/smapbench/parameters/traverse_len
        for I in $(seq 0 30); do cat /sys/module/smapbench/parameters/test_result; done | tr '\n' ' '
        echo
}

# 16 KiB:
run_series $((16 * $KB / 64)) $((32 * $KB * 3048))
# 256 KiB:
run_series $((250 * $KB / 64)) $((256 * $KB * 512))
# 1 MiB:
run_series $((900 * $KB / 64)) $((1024 * $KB * 23))
# 6 MiB:
run_series $(((5 * $MB + 500 * $KB) / 64)) $((6 * $MB * 3))
# 19.25 MiB
run_series $((18 * $MB / 64)) $((18 * $MB * 1))
# 32 MiB
run_series $((32 * $MB / 64)) $((6 * $MB * 1))
```

Each resulting row represents timings for a respective working set size
