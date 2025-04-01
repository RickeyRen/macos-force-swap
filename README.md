# macOS Force Swap Tool

<div align="right">
  <a href="#macos-force-swap-tool">English</a> | 
  <a href="#macos-强制使用swap工具">中文</a>
</div>

This tool is specifically designed to bypass macOS memory compression optimization and force the use of swap files. Unlike ordinary memory allocation tools, this tool uses multiple techniques to ensure memory is actually written to swap files.

## Compilation

```bash
gcc -o force_swap force_swap.c
```

## Usage

```bash
./force_swap [-p percentage] [-c chunk_size_mb] [-s sleep_interval] [-r rounds] [-l lock_enforce]
```

### Parameters

- `-p percentage`: Percentage of physical memory to allocate (default: 300%)
- `-c chunk_size_mb`: Size of each memory chunk allocation (MB) (default: 128MB)
- `-s sleep_interval`: Sleep interval after each allocation (seconds) (default: 0 seconds)
- `-r rounds`: Number of rounds for memory recycling and reallocation (default: 5 rounds)
- `-l lock_enforce`: Whether to enable memory locking to prevent compression (default: 1 enabled)
- `-h`: Display help information

### Examples

1. Use default parameters:
   ```bash
   ./force_swap
   ```

2. Allocate 200% of physical memory, 64MB each time, 3 rounds:
   ```bash
   ./force_swap -p 200 -c 64 -r 3
   ```

3. Allocate 400% of physical memory, disable locking, 10 rounds:
   ```bash
   ./force_swap -p 400 -l 0 -r 10
   ```

## Differences from Ordinary Memory Consumption Tools

This tool is specifically designed to bypass macOS memory optimization mechanisms:

1. **Using mmap instead of malloc**: More direct control over memory allocation
2. **Random data filling**: Generate random data for each memory page to reduce compression efficiency
3. **Forced page eviction**: Use MADV_PAGEOUT to actively evict pages to swap
4. **Multiple rounds of allocation**: Increase the probability of swap usage by releasing and reallocating memory
5. **Real-time monitoring**: Display real-time changes in swap usage

## Technical Details

The tool uses the following techniques to force swap usage:

- **MAP_NORESERVE flag**: Tell the system not to reserve swap space
- **madvise**: Use MADV_PAGEOUT to hint the system to move pages to swap
- **Page-level random data**: Each memory page is filled with different random data to prevent memory compression from merging similar pages
- **Memory locking**: Optionally use mlockall to prevent important memory from being swapped

## Notes

- This tool will cause the system to slow down significantly; make sure to save important work before using
- Root privileges may be required to use certain memory management features
- Heavy use of swap may lead to increased disk space usage
- The program will automatically stop when it cannot allocate more memory
- Press Enter to release all occupied memory and exit the program

---

# macOS 强制使用Swap工具

<div align="right">
  <a href="#macos-force-swap-tool">English</a> | 
  <a href="#macos-强制使用swap工具">中文</a>
</div>

这是一个专门设计用于绕过macOS内存压缩优化，强制使用swap的工具。与普通内存分配工具不同，这个工具使用了多种技术确保内存真正写入swap文件。

## 编译

```bash
gcc -o force_swap force_swap.c
```

## 使用方法

```bash
./force_swap [-p percentage] [-c chunk_size_mb] [-s sleep_interval] [-r rounds] [-l lock_enforce]
```

### 参数说明

- `-p percentage`: 要分配的物理内存百分比（默认：300%）
- `-c chunk_size_mb`: 每次分配的内存块大小（MB）（默认：128MB）
- `-s sleep_interval`: 每次分配后的休眠间隔（秒）（默认：0秒）
- `-r rounds`: 内存回收和重新分配的轮数（默认：5轮）
- `-l lock_enforce`: 是否启用内存锁定防止压缩（默认：1启用）
- `-h`: 显示帮助信息

### 示例

1. 使用默认参数：
   ```bash
   ./force_swap
   ```

2. 分配200%的物理内存，每次64MB，3轮：
   ```bash
   ./force_swap -p 200 -c 64 -r 3
   ```

3. 分配400%的物理内存，禁用锁定，10轮：
   ```bash
   ./force_swap -p 400 -l 0 -r 10
   ```

## 与普通内存占用工具的区别

这个工具专门设计用于绕过macOS的内存优化机制：

1. **使用mmap而非malloc**：更直接地控制内存分配
2. **随机数据填充**：为每个内存页生成随机数据，降低压缩效率
3. **强制页面驱逐**：使用MADV_PAGEOUT主动将页面驱逐到swap
4. **多轮分配**：通过释放和重新分配内存，增加swap使用概率
5. **实时监控**：显示swap使用量的实时变化

## 技术细节

工具使用了以下技术来强制使用swap：

- **MAP_NORESERVE标志**：告诉系统不预留交换空间
- **madvise**：使用MADV_PAGEOUT提示系统将页面移到swap
- **页面级随机数据**：每个内存页填充不同的随机数据，避免内存压缩将相似页面合并
- **内存锁定**：可选地使用mlockall防止重要内存被交换

## 注意事项

- 此工具会导致系统严重变慢，使用时请确保保存重要工作
- 可能需要root权限才能使用某些内存管理功能
- 大量使用swap可能导致系统磁盘空间占用增加
- 程序会在无法分配更多内存时自动停止
- 按Enter键释放所有占用的内存并退出程序 