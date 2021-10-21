#!/bin/bash
echo "umount -l /home/mountdir"
umount -l /home/mountdir
echo "rm -rf /mnt/pmem/root.dat"
rm -f /mnt/pmem/root.dat ;
echo "rm -rf /mnt/pmem/*"
rm -rf /mnt/pmem/* ;
echo "rm -rf /home/mountdir/* "
rm -rf /home/mountdir/* ;
# echo "umount -l ./mountdir"
# umount -l ./mountdir