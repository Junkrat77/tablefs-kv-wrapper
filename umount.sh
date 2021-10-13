#!/bin/bash
echo "rm -rf /mnt/pmem/*"
rm -rf /mnt/pmem/*
echo "rm -rf /mnt/pmem/root.dat"
rm -f /mnt/pmem/root.dat
echo "rm -rf ./mountdir/* "
rm -rf ./mountdir/* 
echo "umount -l ./mountdir"
umount -l ./mountdir