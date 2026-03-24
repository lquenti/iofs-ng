#!/bin/bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
BENCH_DIR="${SCRIPT_DIR}/mnt"
CONFIG_PREFIX="vanilla_"

rm -rf $BENCH_DIR
mkdir $BENCH_DIR
# Latest stable
wget https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.19.9.tar.xz
hyperfine --runs 10 \
  --export-json "${CONFIG_PREFIX}case3_tar_results.json" \
  --prepare "rm -rf ${BENCH_DIR}/* && sync && echo 3 | sudo tee /proc/sys/vm/drop_caches" \
  "tar xf linux-6.19.9.tar.xz -C ${BENCH_DIR}" | tee "${CONFIG_PREFIX}case3_tar_console.txt"
rm linux-6.19.9.tar.xz
