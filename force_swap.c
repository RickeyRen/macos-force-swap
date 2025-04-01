#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/sysctl.h>
#include <time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

// 获取系统物理内存大小（字节）
unsigned long long get_physical_memory() {
    int mib[2] = {CTL_HW, HW_MEMSIZE};
    unsigned long long physical_memory;
    size_t length = sizeof(physical_memory);
    
    if (sysctl(mib, 2, &physical_memory, &length, NULL, 0) == -1) {
        perror("Failed to get physical memory size");
        return 0;
    }
    
    return physical_memory;
}

// 将字节转换为人类可读的格式
void format_size(unsigned long long bytes, char *result) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = bytes;
    
    while (size >= 1024 && unit < 4) {
        size /= 1024;
        unit++;
    }
    
    sprintf(result, "%.2f %s", size, units[unit]);
}

// 获取当前swap使用量
unsigned long long get_swap_usage() {
    FILE *fp = popen("sysctl vm.swapusage | awk '{print $4}'", "r");
    if (fp == NULL) {
        perror("Failed to execute command");
        return 0;
    }
    
    char output[128];
    fgets(output, sizeof(output), fp);
    pclose(fp);
    
    // 解析输出格式 "xxx.xxM"
    float swap_used = 0;
    sscanf(output, "%fM", &swap_used);
    return (unsigned long long)(swap_used * 1024 * 1024);
}

int main(int argc, char *argv[]) {
    // 默认参数
    unsigned long percentage = 300;  // 默认尝试分配物理内存的300%
    unsigned long chunk_size_mb = 128;  // 默认每次分配128MB
    unsigned long sleep_interval = 0;  // 默认每次分配后不睡眠
    unsigned long rounds = 5;        // 默认回收重分配轮数
    int enforce_lock = 1;           // 默认强制页面锁定
    
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            percentage = strtoul(argv[i + 1], NULL, 10);
            i++;
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            chunk_size_mb = strtoul(argv[i + 1], NULL, 10);
            i++;
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            sleep_interval = strtoul(argv[i + 1], NULL, 10);
            i++;
        } else if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) {
            rounds = strtoul(argv[i + 1], NULL, 10);
            i++;
        } else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
            enforce_lock = strtoul(argv[i + 1], NULL, 10);
            i++;
        } else if (strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [-p percentage] [-c chunk_size_mb] [-s sleep_interval] [-r rounds] [-l lock_enforce]\n", argv[0]);
            printf("  -p percentage     Percentage of physical memory to allocate (default: 300%%)\n");
            printf("  -c chunk_size_mb  Size of each memory chunk in MB (default: 128MB)\n");
            printf("  -s sleep_interval Sleep interval between allocations in seconds (default: 0s)\n");
            printf("  -r rounds         Number of memory reclaim and reallocate rounds (default: 5)\n");
            printf("  -l lock_enforce   Whether to enforce memory locking to prevent compression (default: 1)\n");
            printf("  -h                Display this help message\n");
            return 0;
        }
    }
    
    // 获取物理内存大小
    unsigned long long physical_memory = get_physical_memory();
    if (physical_memory == 0) {
        return 1;
    }
    
    // 确保进程在swap使用之前退出不会消除swap
    if (mlockall(MCL_CURRENT) && enforce_lock) {
        printf("Warning: Failed to lock process memory: %s\n", strerror(errno));
        printf("This might reduce swap usage effectiveness\n");
    }
    
    // 计算目标分配大小
    unsigned long long target_allocation = (physical_memory * percentage) / 100;
    unsigned long long chunk_size = chunk_size_mb * 1024 * 1024;
    unsigned long chunks_needed = target_allocation / chunk_size;
    
    char phys_mem_str[20], target_str[20], chunk_str[20];
    format_size(physical_memory, phys_mem_str);
    format_size(target_allocation, target_str);
    format_size(chunk_size, chunk_str);
    
    printf("Physical Memory: %s\n", phys_mem_str);
    printf("Target Allocation: %s (%lu%% of physical memory)\n", target_str, percentage);
    printf("Chunk Size: %s\n", chunk_str);
    printf("Chunks to Allocate: %lu\n", chunks_needed);
    printf("Rounds of Reallocation: %lu\n", rounds);
    printf("Memory Lock Enforce: %s\n", enforce_lock ? "Yes" : "No");
    
    // 显示初始swap使用量
    unsigned long long initial_swap = get_swap_usage();
    char initial_swap_str[20];
    format_size(initial_swap, initial_swap_str);
    printf("Initial Swap Usage: %s\n\n", initial_swap_str);
    
    // 随机种子
    srand(time(NULL));
    
    // 多轮循环强化swap使用
    for (unsigned long round = 0; round < rounds; round++) {
        printf("===== 分配回合 %lu/%lu =====\n", round + 1, rounds);
        
        // 创建指针数组存储分配的内存块
        void **memory_blocks = (void **)malloc(chunks_needed * sizeof(void *));
        if (!memory_blocks) {
            perror("Failed to allocate memory for pointer array");
            return 1;
        }
        
        unsigned long long total_allocated = 0;
        
        // 分配内存并填充复杂的随机数据
        for (unsigned long i = 0; i < chunks_needed; i++) {
            printf("Allocating chunk %lu/%lu... ", i + 1, chunks_needed);
            fflush(stdout);
            
            // 使用mmap而不是malloc，并指定MAP_NORESERVE
            memory_blocks[i] = mmap(NULL, chunk_size, 
                                   PROT_READ | PROT_WRITE, 
                                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, 
                                   -1, 0);
                                   
            if (memory_blocks[i] == MAP_FAILED) {
                printf("FAILED! Memory allocation limit reached.\n");
                chunks_needed = i;
                break;
            }
            
            // 填充复杂随机数据，强制物理内存分配
            for (unsigned long j = 0; j < chunk_size; j += 4096) {
                // 每个内存页填充不同的数据，确保不会被压缩
                unsigned char *page = (unsigned char *)memory_blocks[i] + j;
                for (int k = 0; k < 4096; k++) {
                    page[k] = (unsigned char)(rand() % 256);
                }
            }
            
            // 对于部分内存块，强制驱逐到swap
            if (i % 3 == 0 && i > chunks_needed / 3) {
                if (enforce_lock) {
                    if (madvise(memory_blocks[i], chunk_size, MADV_PAGEOUT) != 0) {
                        printf(" (无法强制驱逐到swap: %s)", strerror(errno));
                    } else {
                        printf(" (强制驱逐到swap)");
                    }
                }
            }
            
            total_allocated += chunk_size;
            char allocated_str[20];
            format_size(total_allocated, allocated_str);
            printf(" SUCCESS! Total allocated: %s\n", allocated_str);
            
            // 睡眠指定的时间间隔
            if (sleep_interval > 0) {
                sleep(sleep_interval);
            }
            
            // 每分配8个块，显示一次当前swap使用量
            if (i % 8 == 7 || i == chunks_needed - 1) {
                unsigned long long current_swap = get_swap_usage();
                char current_swap_str[20];
                format_size(current_swap, current_swap_str);
                printf("Current Swap Usage: %s (增加了 ", current_swap_str);
                format_size(current_swap - initial_swap, current_swap_str);
                printf("%s)\n", current_swap_str);
            }
        }
        
        printf("\n回合 %lu 内存分配完成. %lu 内存块分配.\n", round + 1, chunks_needed);
        
        // 显示总swap使用量
        unsigned long long current_swap = get_swap_usage();
        char current_swap_str[20];
        format_size(current_swap, current_swap_str);
        printf("当前Swap使用量: %s (增加了 ", current_swap_str);
        format_size(current_swap - initial_swap, current_swap_str);
        printf("%s)\n", current_swap_str);
        
        // 如果不是最后一轮，释放内存以便重新分配
        if (round < rounds - 1) {
            printf("释放内存，准备下一轮分配...\n\n");
            for (unsigned long i = 0; i < chunks_needed; i++) {
                if (memory_blocks[i] != MAP_FAILED) {
                    munmap(memory_blocks[i], chunk_size);
                }
            }
        } else {
            printf("\n全部分配轮数完成。正持有内存...\n");
        }
        
        free(memory_blocks);
    }
    
    // 显示最终swap使用量
    unsigned long long final_swap = get_swap_usage();
    char final_swap_str[20];
    format_size(final_swap, final_swap_str);
    printf("\n最终Swap使用量: %s (增加了 ", final_swap_str);
    format_size(final_swap - initial_swap, final_swap_str);
    printf("%s)\n", final_swap_str);
    
    printf("按Enter键释放内存并退出\n");
    getchar();
    
    return 0;
} 