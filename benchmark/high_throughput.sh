#!/bin/bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
BENCH_DIR="${SCRIPT_DIR}/mnt"
CONFIG_PREFIX="vanilla_"

rm -rf $BENCH_DIR/*
sync && echo 3 | sudo tee /proc/sys/vm/drop_caches
fio --name=seq_rw --directory="${BENCH_DIR}" --ioengine=psync --rw=readwrite \
    --bs=1M --size=2G --numjobs=1 --group_reporting --time_based --runtime=600 \
    --output-format=json | tee "${CONFIG_PREFIX}case1_seq_rw_results.json"
