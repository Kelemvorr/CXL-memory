# CXL Memory 侧信道安全检测框架

## 概述

这是一个为 Linux 系统上的 CXL（Compute Express Link）Memory 侧信道安全问题检测而设计的 C 语言框架。该框架提供模块化、可组合的API，用于执行和分析各种侧信道攻击。

## 系统要求

### 硬件配置
- **CPU**: 支持 64 核心（适配 72 核心系统）
- **NUMA**: 两个 NUMA 节点
  - 节点 0: 普通 NUMA 节点，72 核心
  - 节点 1: CXL Memory 节点
- **缓存**: L3 缓存（用于侧信道测量）
- **特性**: 支持 RDTSCP, CLFLUSH 指令

### 软件要求
- Linux 内核 5.0+
- GCC 7.0 或更新版本
- libnuma 开发库
- POSIX 线程库

### 依赖安装（Ubuntu/Debian）

```bash
sudo apt-get install build-essential libnuma-dev numactl
```

## 目录结构

```
cxl_security/linux/
├── include/                           # 头文件目录
│   ├── cxl_common.h                  # 通用定义和内联函数
│   ├── cxl_prepreparation.h          # 参数初始化模块
│   ├── cxl_attack_primitives.h       # 侧信道攻击原语
│   ├── cxl_victim.h                  # 受害者操作接口
│   ├── cxl_attacker.h                # 攻击者操作接口
│   ├── cxl_observation.h             # 观测模块
│   └── cxl_analysis.h                # 分析和可视化模块
├── src/                              # 实现文件
│   ├── cxl_common.c
│   ├── cxl_prepreparation.c
│   ├── cxl_attack_primitives.c
│   ├── cxl_victim.c
│   ├── cxl_attacker.c
│   ├── cxl_observation.c
│   ├── cxl_analysis.c
│   └── cxl_framework.c               # 主框架和演示
├── Makefile
├── README.md                         # 本文件
└── API_REFERENCE.md                  # 详细的 API 文档
```

## 快速开始

### 1. 编译框架

```bash
cd linux
make clean
make all
```

或者一步到位：
```bash
make run
```

### 2. 运行演示

```bash
./bin/cxl_framework
```

输出将显示系统信息和测试结果。

### 3. 查看结果

结果保存在 `results/` 目录下：
- `attack_report.txt` - 攻击成功率报告
- `results.json` - JSON 格式的详细结果
- `*.csv` - 时间序列数据

## 模块说明

### 1. prepreparation 模块（参数初始化）

负责框架参数的初始化和系统配置。

**主要功能：**
- NUMA 节点配置
- 线程位置配置（跨核心、不同线程、同线程）
- 数据放置策略（普通节点、CXL Memory、L3 缓存）
- CPU 亲和性绑定
- 预取器配置
- isolcpus 隔离配置

**使用示例：**
```c
cxl_config_t config;
cxl_config_init(&config);
cxl_set_numa_nodes(&config, 0, 1);
cxl_set_thread_placement(&config, CROSS_CORE);
cxl_set_data_placement(&config, PLACEMENT_CXL_MEMORY);
cxl_print_config(&config);
```

### 2. execute 模块（执行攻击）

分为受害者和攻击者两部分，支持模块化的侧信道攻击。

#### 2.1 attack_primitives 子模块

提供基础的侧信道操作原语：

- **Flush 操作**: `cxl_flush_clflush()`, `cxl_flush_clflushopt()`, `cxl_flush_range()`
- **Probe 操作**: `cxl_probe_access_time()`, `cxl_probe_multiple()`
- **Reload 操作**: `cxl_reload()`
- **组合攻击**: `cxl_flush_reload()`, `cxl_evict_time()`, `cxl_spectre_variant()`

**使用示例：**
```c
/* Flush + Reload 攻击 */
cxl_flush_clflush(data_address);
cxl_mfence();
/* 给受害者时间访问数据 */
usleep(100);
uint64_t access_time = cxl_probe_access_time(data_address, NULL);
uint64_t threshold = cxl_get_timing_threshold();
int is_cached = (access_time < threshold);
```

#### 2.2 victim 模块（受害者）

执行包含敏感数据访问的操作。

**主要操作：**
- `cxl_victim_memory_sequence()` - 执行内存访问序列
- `cxl_victim_single_access()` - 单次内存访问
- `cxl_victim_workload()` - 执行指定的工作负载
- `cxl_victim_encrypt_operation()` - 模拟加密操作（测试表访问泄露）
- `cxl_victim_branch_operation()` - 执行有分支的操作（Spectre 风格）

**使用示例：**
```c
cxl_victim_init(cpu_id);
cxl_victim_single_access(sensitive_data, 0);  /* 读操作 */
```

#### 2.3 attacker 模块（攻击者）

执行各种侧信道攻击。

**支持的攻击：**
- Flush + Reload
- Evict + Time
- Prime + Probe
- Spectre
- Meltdown (演示)
- 时序旁路

**使用示例：**
```c
cxl_attacker_init(cpu_id);
attack_result_t result;
cxl_attacker_flush_reload(victim_data, &result);
float success_rate = cxl_attacker_success_rate(&result, 1);
```

### 3. observation 模块（观测）

收集侧信道信息。

**观测类型：**
- **OBSERVE_TIMING** - 观测访问时间
- **OBSERVE_PATTERN** - 观测缓存命中/未命中模式
- **OBSERVE_TRACE** - 观测内存访问痕迹

**主要函数：**
- `cxl_observe_access_timing()` - 采集访问时间
- `cxl_observe_cache_pattern()` - 采集缓存命中/未命中模式
- `cxl_observe_cxl_latency()` - 测量 CXL Memory 延迟
- `cxl_observe_rdtscp_samples()` - 采集高精度时间戳

**使用示例：**
```c
cxl_observation_init(OBSERVE_TIMING, 1000000);
uint64_t timings[1000];
int num_samples = cxl_observe_access_timing(address, 1000, timings);
cxl_observation_cleanup();
```

### 4. analysis 模块（分析与可视化）

处理和可视化侧信道数据。

**分析功能：**
- 统计分析（最小值、最大值、平均值、标准差、中位数）
- 分布对比和统计显著性检验
- 信号恢复和去噪
- 命中/未命中分离度分析
- CXL Memory 延迟特性分析

**数据导出格式：**
- CSV（时间序列数据）
- JSON（结构化结果）
- 文本报告（人类可读）

**使用示例：**
```c
cxl_analysis_init("./results");

uint64_t min, max;
double mean, median, stddev;
cxl_analysis_compute_statistics(timings, num_samples, 
                                &min, &max, &mean, &median, &stddev);

cxl_analysis_export_csv(timings, num_samples, "access_times", 
                       "results/timings.csv");

cxl_analysis_cleanup();
```

## API 参考

完整的 API 参考请见 [API_REFERENCE.md](API_REFERENCE.md)

## 配置选项

### 线程放置策略

1. **CROSS_CORE** - 攻击者和受害者在不同核心运行
2. **DIFFERENT_THREAD** - 在不同线程中运行（可能同核心）
3. **SAME_THREAD** - 在同一线程（特殊场景）

### 数据放置策略

1. **PLACEMENT_NORMAL_NODE** - 数据放在普通 NUMA 节点 0
2. **PLACEMENT_CXL_MEMORY** - 数据放在 CXL Memory 节点 1
3. **PLACEMENT_LOCAL** - 数据放在 L3 缓存中

## 性能优化

### 关键参数调整

1. **timing_threshold** - 用于判断缓存命中/未命中的时间阈值
   ```c
   cxl_set_timing_threshold(200);  /* 200 cycles */
   ```

2. **测量次数** - 增加样本数量可以提高统计显著性
   ```c
   cxl_observe_access_timing(addr, 10000, timings);  /* 更多样本 */
   ```

3. **隔离** - 启用 isolcpus 减少噪声
   ```bash
   # 在 Linux 启动参数中添加
   isolcpus=40-71
   ```

## 安全注意事项

1. **权限** - 某些操作（如 MSR 访问）需要 root 权限
2. **系统影响** - 大量 clflush 操作可能影响系统性能
3. **隐私** - 框架可用于提取敏感信息，使用需谨慎

## 尝试的攻击场景

### 场景 1: 基本 Flush + Reload
```c
/* 清除缓存 */
cxl_flush_clflush(target_addr);

/* 给受害者时间访问 */
usleep(10);

/* 测量访问时间 */
uint64_t time = cxl_probe_access_time(target_addr, NULL);
```

### 场景 2: CXL Memory 特性识别
```c
/* 分别测量 CXL 和普通内存延迟 */
cxl_observe_cxl_latency(cxl_addr, normal_addr, 
                       cxl_timings, normal_timings, 10000);

/* 分析延迟差异 */
double latency_diff, signal_strength;
cxl_analysis_latency_difference(cxl_timings, normal_timings, 10000,
                               &latency_diff, &signal_strength);
```

### 场景 3: 多线程隔离测试
```c
cxl_setup_multithreading(&config, 16);
/* 创建 16 个工作线程 */
pthread_create(..., cxl_victim_thread_main, ...);
pthread_create(..., cxl_attacker_thread_main, ...);
```

## 故障排除

### 编译错误

**错误**: `fatal error: numa.h: No such file or directory`
**解决**: `sudo apt-get install libnuma-dev`

**错误**: NUMA 节点不足
**解决**: 框架自动降级为单节点模式

### 运行时问题

**问题**: 攻击成功率为 0%
**解决**: 
1. 增加 timing_threshold
2. 增加样本数量
3. 检查 CPU 频率守护程序（cpufreq）

**问题**: 权限不足进行 clflush
**解决**: 在某些内核版本上，clflush 可能需要特殊权限，尝试以 root 运行

## 扩展框架

### 添加自定义攻击

1. 在 `cxl_attack_primitives.h` 中声明函数
2. 在 `cxl_attack_primitives.c` 中实现
3. 在攻击者模块中调用

### 添加自定义观测

1. 在 `cxl_observation.h` 中声明
2. 在 `cxl_observation.c` 中实现观测逻辑
3. 在分析模块中添加对应的处理

## 参考资源

- [CXL 规范](https://www.computeexpresslink.org/)
- [侧信道攻击综述](https://www.usenix.org/system/files/login/articles/10_CPU_Side-Channel_113-120_final.pdf)
- [Spectre and Meltdown Paper](https://spectreattack.com/)

## 许可证

该框架采用 GPL v3.0 许可证。

## 贡献

欢迎提交问题报告和改进建议。

## 作者

CXL 侧信道安全检测框架 v1.0

---

**最后更新**: 2026 年 2 月

**维护者**: CXL Security Research Team
