#!/bin/bash

bench=("createfiles.f" "openfiles.f" "createdelete-swing.f" "listdirs.f")

fs_name=$1

if [ $# -eq 0 ]; then
  echo "FS name is not found"
  exit 0
fi

for bench_file in ${bench[@]}
do
    mount="./tablefs -mountdir /mnt/tablefs -metadir /mnt/pmem/tablefs -datadir /mnt/pmem/tablefs"
    run="filebench -f ${bench_file} | tee ${fs_name}-${bench_file}.log"
    clear="umount /mnt/tablefs && rm -rf /mnt/pmem/${fs_name}/* && rm -rf /mnt/pmem/tablefs/*"
    echo ${mount}
    echo ${run}
    echo ${clear}
done