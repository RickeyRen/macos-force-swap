#!/bin/bash

echo "===== 系统swap使用情况 ====="
sysctl vm.swapusage

echo -e "\n===== 物理内存大小 ====="
physical_memory=$(sysctl -n hw.memsize)
echo "物理内存: $((physical_memory / 1024 / 1024 / 1024)) GB ($(echo $physical_memory | sed ':a;s/\B[0-9]\{3\}\>/,&/;ta') bytes)"

echo -e "\n===== Swap文件位置 ====="
echo "Swap目录软链接:"
ls -la /private/var/vm

echo -e "\nSwap实际目录内容:"
sudo ls -lah "/Volumes/Macintosh HD - Data/VM" 2>/dev/null || echo "无法访问实际swap目录，可能需要sudo权限"

echo -e "\n===== 内存压缩统计 ====="
vm_stat | grep -E "compressor|Compress|Swapins|Swapouts"

echo -e "\n===== 动态创建的swap文件 ====="
echo "macOS会根据需要动态创建swap文件，通常命名为swapfile0, swapfile1, ..."
echo "文件将在'/Volumes/Macintosh HD - Data/VM/'目录中"
echo "目前似乎没有活跃的swap文件"

echo -e "\n===== 手动测试swap使用 ====="
echo "你可以使用以下命令来测试和占用swap:"
echo "./swap_consumer -p 200 -c 256 -s 1"
echo "观察系统活动监视器或使用vm_stat命令查看swap使用变化"

gcc -o force_swap force_swap.c
./force_swap 