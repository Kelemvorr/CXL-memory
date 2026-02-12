# CXL 侧信道检测框架 - API 参考文档

## 目录

1. [通用接口 (cxl_common.h)](#通用接口)
2. [参数初始化 (cxl_prepreparation.h)](#参数初始化)
3. [攻击原语 (cxl_attack_primitives.h)](#攻击原语)
4. [受害者接口 (cxl_victim.h)](#受害者接口)
5. [攻击者接口 (cxl_attacker.h)](#攻击者接口)
6. [观测接口 (cxl_observation.h)](#观测接口)
7. [分析接口 (cxl_analysis.h)](#分析接口)

---

## 通用接口

### 内存管理

#### `void *cxl_malloc_on_node(size_t size, int node)`
在指定的 NUMA 节点上分配内存。

**参数:**
- `size`: 要分配的字节数
- `node`: NUMA 节点 ID（-1 表示默认节点）

**返回值:** 分配的内存指针，失败返回 NULL

**示例:**
```c
void *data = cxl_malloc_on_node(4096, 1);  /* 在节点 1 (CXL) 分配 4KB */
```

#### `void cxl_free(void *ptr, size_t size)`
释放由 `cxl_malloc_on_node` 分配的内存。

### CPU 亲和性

#### `int cxl_bind_to_cpu(int cpu_id)`
将当前线程绑定到指定的 CPU 核心。

**参数:**
- `cpu_id`: 目标 CPU 核心 ID

**返回值:** 0 成功，-1 失败

#### `int cxl_bind_to_node(int node_id)`
将当前线程绑定到指定的 NUMA 节点。

### 时间测量

#### `uint64_t cxl_rdtscp(uint32_t *cpu_id)` (内联)
使用 RDTSCP 指令获取高精度时间戳，同时获得 CPU ID。

**参数:**
- `cpu_id`: 返回当前运行的 CPU ID（可空）

**返回值:** 时间戳计数

**示例:**
```c
uint32_t cpu;
uint64_t ts = cxl_rdtscp(&cpu);
printf("Time: %lu, CPU: %u\n", ts, cpu);
```

#### `uint64_t cxl_access_time(uint64_t start, uint64_t end)` (内联)
计算两个时间戳之间的差异（周期数）。

### 内存屏障

#### `void cxl_mfence(void)` (内联)
完整的内存屏障，确保之前的所有内存操作都已完成。

#### `void cxl_lfence(void)` (内联)
加载屏障，确保之前的所有加载操作都已完成。

#### `void cxl_serialization_point(void)` (内联)
通用序列化点（目前实现为 mfence）。

### 系统信息

#### `int cxl_get_num_cpus(void)`
获取系统 CPU 核心总数。

#### `int cxl_get_num_numa_nodes(void)`
获取系统 NUMA 节点数。

#### `int cxl_check_system_support(void)`
检查系统是否支持 CXL Memory 测试。

**返回值:** 1 支持，0 不支持或警告，-1 错误

#### `int cxl_get_cxl_node(void)`
获取 CXL Memory 节点的 ID。

---

## 参数初始化

### 配置管理

#### `int cxl_config_init(cxl_config_t *config)`
初始化框架配置结构。

**参数:**
- `config`: 指向配置结构的指针

**返回值:** 0 成功，-1 失败

**说明:** 设置默认值（72 核 CPU，2 个 NUMA 节点等）

#### `int cxl_set_numa_nodes(cxl_config_t *config, int normal_node, int cxl_node)`
设置普通 NUMA 节点和 CXL Memory 节点。

#### `int cxl_set_thread_placement(cxl_config_t *config, thread_placement_t placement)`
配置线程位置策略。

**placement 取值:**
- `CROSS_CORE` - 不同核心
- `DIFFERENT_THREAD` - 不同线程
- `SAME_THREAD` - 同一线程

#### `int cxl_set_data_placement(cxl_config_t *config, data_placement_t placement)`
配置数据放置位置。

**placement 取值:**
- `PLACEMENT_NORMAL_NODE` - 普通节点
- `PLACEMENT_CXL_MEMORY` - CXL Memory
- `PLACEMENT_LOCAL` - L3 缓存

#### `int cxl_set_cpu_binding(cxl_config_t *config, int attacker_cpu, int victim_cpu)`
指定攻击者和受害者的 CPU 绑定。

#### `void cxl_print_config(const cxl_config_t *config)`
打印当前配置信息。

#### `int cxl_validate_config(const cxl_config_t *config)`
验证配置的有效性。

### 系统配置

#### `int cxl_configure_prefetcher(int enabled)`
配置 CPU 预取器。

**参数:**
- `enabled`: 1 启用，0 禁用

#### `int cxl_configure_isolcpus(int enabled)`
配置 isolcpus 内核参数。

#### `int cxl_setup_multithreading(cxl_config_t *config, int num_threads)`
配置多线程测试环境。

#### `int cxl_setup_singlethreading(cxl_config_t *config)`
配置单线程隔离测试环境。

---

## 攻击原语

### 缓存清除

#### `void cxl_flush_clflush(void *addr)`
使用 CLFLUSH 指令清除地址对应的缓存行。

#### `void cxl_flush_clflushopt(void *addr)`
使用 CLFLUSHOPT 指令（优化版本，如果支持）。

#### `void cxl_flush_range(void *start_addr, void *end_addr)`
清除地址范围内的所有缓存行。

### 缓存探测

#### `uint64_t cxl_probe_access_time(void *addr, uint64_t *out_time)`
测量访问地址的时间。

**参数:**
- `addr`: 要访问的内存地址
- `out_time`: 返回的访问时间（可空）

**返回值:** 访问时间（周期）

**说明:** 自动执行 lfence 以保证精确测量

#### `void cxl_probe_multiple(void **addrs, int num_addrs, uint64_t *timings)`
批量探测多个地址。

### 缓存重新加载

#### `void cxl_reload(void *addr)`
将地址重新加载到缓存中。

### 侧信道攻击操作

#### `uint64_t cxl_attack_load(void *addr)`
执行加载操作并返回访问时间。

#### `void cxl_attack_store(void *addr, uint64_t value)`
执行存储操作。

#### `void cxl_attack_prefetch(void *addr)`
执行预取操作。

#### `void cxl_attack_mfence(void)`
执行内存屏障。

#### `void cxl_attack_lfence(void)`
执行加载屏障。

### 组合攻击

#### `uint64_t cxl_flush_reload(void *addr)`
执行完整的 Flush + Reload 步骤。

**返回值:** Reload 的访问时间

#### `uint64_t cxl_evict_time(void *evict_addr, void *probe_addr)`
执行 Evict + Time 组合。

#### `uint64_t cxl_spectre_variant(void *cond_addr, void *true_addr, void *probe_addr)`
执行 Spectre 风格的推测执行攻击。

### 参数管理

#### `void cxl_set_timing_threshold(uint64_t threshold)`
设置用于判断缓存命中/未命中的时间阈值。

#### `uint64_t cxl_get_timing_threshold(void)`
获取当前的时间阈值。

---

## 受害者接口

### 初始化

#### `int cxl_victim_init(int cpu_id)`
初始化受害者线程，绑定到指定 CPU。

#### `int cxl_victim_cleanup(void)`
清理受害者资源。

### 内存操作

#### `int cxl_victim_memory_sequence(void **addrs, int num_addrs, const char *access_pattern)`
执行内存访问序列。

**access_pattern 取值:**
- `"sequential"` - 顺序访问
- `"random"` - 随机访问
- `"stride"` - 步长访问

#### `uint64_t cxl_victim_single_access(void *addr, int is_write)`
执行单次内存访问。

**参数:**
- `is_write`: 1 写操作，0 读操作

### 工作负载

#### `int cxl_victim_workload(uint64_t duration_us, const char *workload_type)`
执行指定的工作负载。

**workload_type 取值:**
- `"cpu-intensive"` - CPU 密集型
- `"memory-intensive"` - 内存密集型

#### `int cxl_victim_encrypt_operation(const void *key, const void *plaintext, void *ciphertext)`
模拟加密操作（用于测试表访问泄露）。

#### `int cxl_victim_lookup_operation(void *table, size_t table_size, const uint32_t *indices, int num_indices)`
模拟表查询操作。

#### `int cxl_victim_branch_operation(int condition, void *true_data, void *false_data)`
模拟有分支的操作（用于 Spectre 测试）。

#### `int cxl_victim_chain_access(void **chain_addrs, int chain_size)`
执行链式内存访问。

### 状态管理

#### `int cxl_victim_get_statistics(victim_stats_t *stats)`
获取受害者的访问统计信息。

#### `int cxl_victim_timed_loop(void *addr, uint64_t iterations, uint64_t interval_us)`
执行定时循环访问。

#### `void cxl_victim_idle(uint64_t idle_duration_us)`
使受害者线程空闲指定时间。

---

## 攻击者接口

### 初始化

#### `int cxl_attacker_init(int cpu_id)`
初始化攻击者线程。

#### `int cxl_attacker_cleanup(void)`
清理攻击者资源。

### 攻击方法

#### `int cxl_attacker_flush_reload(void *victim_data, attack_result_t *result)`
执行 Flush + Reload 攻击。

**result 包含:**
- `is_hit`: 缓存命中标志
- `victim_access_time`: 访问时间
- `hit_count`: 累积命中数

#### `int cxl_attacker_evict_time(void *victim_data, void **evict_set, int evict_set_size, attack_result_t *result)`
执行 Evict + Time 攻击。

#### `int cxl_attacker_spectre(void *gadget_addr, int condition, attack_result_t *result)`
执行 Spectre 推测执行攻击。

#### `int cxl_attacker_meltdown(void *target_addr, attack_result_t *result)`
执行 Meltdown 越权读取攻击。

#### `int cxl_attacker_prime_probe(void **target_set, int set_size, attack_result_t *result)`
执行 Prime + Probe 攻击。

### 探测和观测

#### `int cxl_attacker_probe_addresses(void **addrs, int num_addrs, uint64_t *timings, int *num_timings)`
批量探测地址的缓存状态。

#### `int cxl_attacker_timing_sidechannel(void *victim_addr, uint64_t threshold, int num_samples, uint64_t *samples)`
执行时序旁路攻击并收集样本。

#### `int cxl_attacker_collect_pattern(void *victim_addr, int num_probes, uint8_t *pattern)`
收集访问模式（命中/未命中序列）。

### 数据处理

#### `int cxl_attacker_noise_filtering(const uint64_t *raw_samples, int num_samples, uint64_t *filtered_samples)`
对时间测量数据进行去噪滤波。

#### `int cxl_attacker_evict_set(void **evict_set, int set_size)`
执行驱逐集操作。

#### `int cxl_attacker_warmup(uint64_t warmup_iterations)`
预热 CPU 缓存。

### 统计

#### `float cxl_attacker_success_rate(const attack_result_t *results, int num_results)`
计算攻击的成功率。

**返回值:** 0.0 到 1.0 的成功率

---

## 观测接口

### 初始化

#### `int cxl_observation_init(observation_type_t observation_type, size_t buffer_size)`
初始化观测模块。

**observation_type 取值:**
- `OBSERVE_TIMING` - 时间观测
- `OBSERVE_PATTERN` - 模式观测
- `OBSERVE_TRACE` - 痕迹观测

#### `int cxl_observation_cleanup(void)`
清理观测模块。

### 时间观测

#### `int cxl_observe_access_timing(void *addr, int num_samples, uint64_t *samples)`
采集内存访问时间。

**返回值:** 实际采集的样本数

#### `int cxl_observe_rdtscp_samples(timing_sample_t *samples, int num_samples)`
采集高精度时间戳样本。

#### `int cxl_observe_access_intervals(void *addr, int max_intervals, uint64_t *intervals)`
观测重复访问之间的时间间隔。

### 模式观测

#### `int cxl_observe_cache_pattern(void **addrs, int num_addrs, int num_probes, uint8_t *hit_patterns)`
观测缓存命中/未命中模式。

#### `int cxl_observe_access_trace(void *addr_range_start, size_t addr_range_size, int *num_traces, uint32_t *traces)`
观测内存访问痕迹。

### CXL 特性观测

#### `int cxl_observe_cxl_latency(void *cxl_addr, void *normal_addr, uint64_t *cxl_timings, uint64_t *normal_timings, int num_samples)`
测量 CXL Memory 与普通内存的延迟差异。

#### `int cxl_observe_l3_timing(void *addr, int num_measurements, uint64_t *timings)`
观测 L3 缓存的时间特性。

### 其他观测

#### `int cxl_observe_cache_conflicts(void **addrs, int num_addrs, uint8_t *conflicts)`
检测缓存冲突。

#### `int cxl_observe_timing_overlap(uint64_t attacker_timing, uint64_t victim_timing, uint64_t *overlap_time)`
检测攻击者和受害者的时间重叠。

#### `int cxl_observe_statistics(const uint64_t *samples, int num_samples, uint64_t *min, uint64_t *max, double *mean, double *stddev)`
计算统计信息。

#### `int cxl_observe_detect_anomalies(const uint64_t *samples, int num_samples, double threshold, uint8_t *anomalies)`
检测异常值。

### 实时观测

#### `int cxl_observe_realtime_start(void *target_addr, observation_type_t observation_type, observation_callback_t callback, void *ctx)`
启动实时观测线程。

#### `int cxl_observe_realtime_stop(void)`
停止实时观测。

### 缓冲区管理

#### `int cxl_observation_clear_buffer(void)`
清空观测缓冲区。

#### `int cxl_observation_get_data(void *buffer, size_t buffer_size)`
获取观测缓冲区中的数据。

---

## 分析接口

### 初始化

#### `int cxl_analysis_init(const char *output_dir)`
初始化分析模块。

#### `int cxl_analysis_cleanup(void)`
清理分析模块。

### 基本统计

#### `int cxl_analysis_compute_statistics(const uint64_t *timings, int num_samples, uint64_t *min, uint64_t *max, double *mean, double *median, double *stddev)`
计算时间序列的统计信息。

#### `int cxl_analysis_compare_distributions(const uint64_t *timings_a, int num_a, const uint64_t *timings_b, int num_b, double *difference, double *p_value)`
对比两个分布。

### 数据导出

#### `int cxl_analysis_export_csv(const uint64_t *timings, int num_samples, const char *label, const char *output_file)`
导出 CSV 格式数据。

#### `int cxl_analysis_export_json(const attack_result_t *results, int num_results, const char *output_file)`
导出 JSON 格式数据。

### 可视化

#### `int cxl_analysis_histogram(const uint64_t *timings, int num_samples, int num_buckets, uint32_t *histogram)`
生成直方图数据。

#### `char *cxl_analysis_ascii_plot(const uint64_t *timings, int num_samples, int width, int height)`
生成 ASCII 格式的绘图。

#### `int cxl_analysis_heatmap(const double *data, int rows, int cols, const char *output_file)`
生成热图数据。

### 高级分析

#### `int cxl_analysis_latency_difference(const uint64_t *cxl_timings, const uint64_t *normal_timings, int num_samples, double *latency_diff, double *signal_strength)`
分析 CXL 与普通内存的延迟差异。

#### `int cxl_analysis_hit_miss_separation(const uint64_t *hit_timings, int num_hits, const uint64_t *miss_timings, int num_misses, double *separation_ratio)`
计算命中/未命中的分离度。

#### `uint64_t cxl_analysis_recommend_threshold(const uint64_t *hit_timings, int num_hits, const uint64_t *miss_timings, int num_misses)`
推荐最优的测时阈值。

#### `int cxl_analysis_signal_recovery(const uint64_t *raw_samples, int num_samples, const char *filter_type, uint64_t *filtered_samples)`
从噪声中恢复信号。

### 报告生成

#### `int cxl_analysis_attack_success_report(const attack_result_t *results, int num_results, int num_data_placement_types, int num_thread_configs, const char *output_file)`
生成攻击成功率报告。

#### `int cxl_analysis_full_report(const attack_result_t *results, int num_results, const cxl_config_t *config, const char *output_dir)`
生成完整的分析报告。

#### `int cxl_analysis_plot_success_curve(const float *success_rates, int num_points, const char *output_file)`
绘制攻击成功率曲线。

---

## 数据结构

### `cxl_config_t`
框架配置结构：
```c
typedef struct {
    int numa_node_normal;
    int numa_node_cxl;
    thread_placement_t thread_placement;
    data_placement_t data_placement;
    int attacker_cpu;
    int victim_cpu;
    int probe_cpu;
    int monitor_cpu;
    int prefetcher_enabled;
    int isolcpus_enabled;
    uint64_t iterations;
    uint64_t warmup_iterations;
    uint32_t sample_size;
} cxl_config_t;
```

### `attack_result_t`
攻击结果结构：
```c
typedef struct {
    uint64_t attack_id;
    uint64_t victim_access_time;
    uint64_t attacker_probe_time;
    uint64_t latency_diff;
    uint32_t hit_count;
    uint32_t miss_count;
    uint8_t is_hit;
    data_placement_t data_location;
    thread_placement_t thread_config;
} attack_result_t;
```

### `observation_data_t`
观测数据结构：
```c
typedef struct {
    uint64_t sample_id;
    uint64_t timestamp;
    uint64_t access_time;
    uint32_t cpu_id;
    uint8_t is_hit;
    uint64_t address;
} observation_data_t;
```

### `timing_sample_t`
时间戳样本结构：
```c
typedef struct {
    uint64_t tsc;
    uint64_t apic_id;
    uint64_t timestamp;
} timing_sample_t;
```

### `victim_stats_t`
受害者统计信息：
```c
typedef struct {
    uint64_t total_accesses;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t avg_latency;
    uint64_t max_latency;
    uint64_t min_latency;
} victim_stats_t;
```

---

**文档版本:** 1.0  
**最后更新:** 2026 年 2 月
