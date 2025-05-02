#include <algorithm>
#include "stats.h"

/*
 (EN) Definition of the global statistics variables
 (CN) 全局统计变量的定义
*/

// (EN) LLC counters
// (CN) LLC计数
uint64_t llc_total_miss = 0;
uint64_t llc_total_access = 0;

// (EN) DRAM bytes, sim cycle counters
// (CN) DRAM字节数,模拟周期等计数
uint64_t dram_bytes_transferred = 0;
uint64_t simulation_cycles = 0;
uint64_t cpu_frequency = 4000;  // (EN) example: 4000 MHz (CN) 示例: 4000 MHz

// (EN) Branch misprediction stats
// (CN) 分支预测失误的统计
uint64_t branch_mispred_count = 0;
uint64_t branch_total_count = 0;

/*
 (EN) get_llc_miss_rate():
      Return ratio of llc_total_miss / (llc_total_access + 1e-9)
      we add 1e-9 to avoid division by zero
 (CN) get_llc_miss_rate():
      返回 llc_total_miss / (llc_total_access + 1e-9)
      用 1e-9 避免除零错误
*/
double get_llc_miss_rate() {
    return static_cast<double>(llc_total_miss) / (llc_total_access + 1e-9);
}

/*
 (EN) get_bw_usage():
      1) compute simulation_time = simulation_cycles / (cpu_frequency * 1e6)
      2) bytes per second = dram_bytes_transferred / simulation_time
      3) convert to GB/s => / 1e9
 (CN) get_bw_usage():
      1) 计算模拟时间 = simulation_cycles / (cpu_frequency * 1e6)
      2) 每秒传输字节 = dram_bytes_transferred / simulation_time
      3) 转换为GB/s => 除以1e9
*/
double get_bw_usage() {
    double simulation_time = static_cast<double>(simulation_cycles) / (cpu_frequency * 1e6);
    double bw_bytes_per_sec = static_cast<double>(dram_bytes_transferred) / (simulation_time + 1e-9);
    return bw_bytes_per_sec / 1e9;  // (EN) convert to GB/s (CN) 转换为GB/s
}

/*
 (EN) get_mispred_rate():
      Return ratio of branch_mispred_count / (branch_total_count + 1e-9)
 (CN) get_mispred_rate():
      返回 branch_mispred_count / (branch_total_count + 1e-9)
*/
double get_mispred_rate() {
    return static_cast<double>(branch_mispred_count) / (branch_total_count + 1e-9);
}

Stats stats = {}; // 初始化所有成员为0