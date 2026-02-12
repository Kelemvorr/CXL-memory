# CXL Memory 侧信道安全检测框架 - 项目总结

## 项目概述

本项目为 Linux 系统上的 CXL（Compute Express Link）Memory 侧信道安全问题检测实现了一个完整的 C 语言框架。该框架采用模块化设计，提供了灵活可组合的 API 用于执行和分析各种侧信道攻击。

## 框架架构

### 四大核心模块

```
┌─────────────────────────────────────────────────────┐
│           CXL 侧信道检测框架树形结构              │
├─────────────────────────────────────────────────────┤
│  1. PREPREPARATION (参数初始化)                     │
│     ├─ NUMA 节点配置                                │
│     ├─ 线程位置配置                                │
│     ├─ 数据放置策略                                │
│     ├─ CPU 亲和性绑定                              │
│     └─ 系统参数配置                                │
│                                                     │
│  2. EXECUTE (执行攻击)                              │
│     ├─ Attack Primitives (基础原语)                │
│     │  ├─ Flush 操作 (clflush, clflushopt)        │
│     │  ├─ Probe 操作 (访问时间测量)               │
│     │  ├─ Reload 操作                             │
│     │  └─ 组合攻击 (Flush+Reload, Evict+Time 等)  │
│     ├─ Victim (受害者)                            │
│     │  ├─ 内存序列访问                            │
│     │  ├─ 工作负载执行                            │
│     │  ├─ 加密操作模拟                            │
│     │  └─ 分支操作 (Spectre 测试)                 │
│     └─ Attacker (攻击者)                          │
│        ├─ Flush + Reload 攻击                     │
│        ├─ Evict + Time 攻击                       │
│        ├─ Prime + Probe 攻击                      │
│        ├─ Spectre 攻击                           │
│        ├─ Meltdown 攻击                          │
│        └─ 时序旁路攻击                           │
│                                                     │
│  3. OBSERVATION (观测)                              │
│     ├─ 时间观测 (RDTSCP 采样)                      │
│     ├─ 模式观测 (缓存命中/未命中)                 │
│     ├─ 延迟观测 (CXL vs 普通内存)                 │
│     ├─ 痕迹观测 (访问地址)                        │
│     ├─ L3 时间特性观测                            │
│     ├─ 冲突检测                                   │
│     └─ 实时观测线程                               │
│                                                     │
│  4. ANALYSIS (分析与可视化)                         │
│     ├─ 统计分析 (min, max, mean, median, stddev) │
│     ├─ 分布对比 (t-test)                         │
│     ├─ 信号恢复 (去噪滤波)                         │
│     ├─ 数据导出 (CSV, JSON)                       │
│     ├─ 可视化 (直方图, 热图, ASCII 图)            │
│     ├─ 报告生成                                   │
│     └─ 成功率分析                                 │
└─────────────────────────────────────────────────────┘
```

## 文件结构清单

```
CXL_security/linux/
│
├── include/                          (7 个头文件)
│   ├── cxl_common.h                 - 通用定义和内联函数
│   ├── cxl_prepreparation.h         - 参数初始化 API
│   ├── cxl_attack_primitives.h      - 侧信道攻击原语 API
│   ├── cxl_victim.h                 - 受害者操作 API
│   ├── cxl_attacker.h               - 攻击者操作 API
│   ├── cxl_observation.h            - 观测模块 API
│   └── cxl_analysis.h               - 分析与可视化 API
│
├── src/                             (7 个实现文件)
│   ├── cxl_common.c                 - 通用函数实现 (~200 行)
│   ├── cxl_prepreparation.c         - 初始化模块实现 (~250 行)
│   ├── cxl_attack_primitives.c      - 攻击原语实现 (~300 行)
│   ├── cxl_victim.c                 - 受害者实现 (~350 行)
│   ├── cxl_attacker.c               - 攻击者实现 (~400 行)
│   ├── cxl_observation.c            - 观测模块实现 (~500 行)
│   ├── cxl_analysis.c               - 分析模块实现 (~600 线)
│   └── cxl_framework.c              - 主框架和演示程序 (~300 行)
│
├── Makefile                          - 编译配置
├── README.md                         - 使用文档
├── API_REFERENCE.md                  - 详细 API 参考
└── verify_build.sh                   - 编译验证脚本
```

**代码统计:**
- 总文件数: 15 个 (7 个头文件 + 8 个源文件)
- 总代码行数: 约 2700+ 行 C 代码
- API 函数数: 100+ 个

## 关键特性

### 1. 模块化设计
- 每个模块独立设计，相互解耦
- 每个 .c 文件对应一个 .h 文件
- 通过清晰的 API 接口进行通信

### 2. 支持的攻击向量
| 攻击名称 | 目的 | 难度 |
|---------|------|------|
| Flush + Reload | 检测缓存访问 | 中等 |
| Evict + Time | 通过驱逐检测访问 | 中等 |
| Prime + Probe | 检测共享缓存使用 | 困难 |
| Spectre | 利用推测执行 | 困难 |
| Meltdown | 越权内存访问 | 困难 |
| 时序旁路 | 通过延迟泄露信息 | 简单 |

### 3. 灵活的配置选项
- **线程位置**: 跨核心、不同线程、同线程
- **数据位置**: 普通 NUMA、CXL Memory、L3 缓存
- **系统配置**: 预取器、isolcpus 隔离
- **工作参数**: 迭代次数、样本大小、阈值

### 4. 完整的观测能力
- 高精度时间戳 (RDTSCP)
- 缓存命中/未命中检测
- CXL Memory 延迟特征识别
- 实时观测线程

### 5. 强大的分析功能
- 统计分析 (平均值、标准差、中位数)
- 信号处理 (去噪、滤波)
- 可视化 (CSV、JSON、直方图)
- 自动阈值推荐

## 编译与运行

### 编译
```bash
cd linux
make clean
make all
```

### 运行演示
```bash
./bin/cxl_framework
```

### 查看结果
```bash
ls -la results/
cat results/attack_report.txt
```

## 使用示例

### 示例 1: 基本的 Flush + Reload 攻击
```c
#include "cxl_framework.h"

int main() {
    cxl_config_t config;
    cxl_config_init(&config);
    cxl_set_numa_nodes(&config, 0, 1);
    
    void *data = malloc(4096);
    
    cxl_attacker_init(0);
    attack_result_t result;
    cxl_attacker_flush_reload(data, &result);
    
    printf("Hit: %d, Time: %lu\n", result.is_hit, result.attacker_probe_time);
    
    cxl_attacker_cleanup();
    free(data);
    return 0;
}
```

### 示例 2: CXL Memory 延迟测试
```c
void *cxl_addr = cxl_malloc_on_node(4096, 1);
void *normal_addr = malloc(4096);

uint64_t cxl_times[1000], normal_times[1000];
cxl_observe_cxl_latency(cxl_addr, normal_addr,
                       cxl_times, normal_times, 1000);

double latency_diff, signal_strength;
cxl_analysis_latency_difference(cxl_times, normal_times, 1000,
                               &latency_diff, &signal_strength);

printf("Latency difference: %.2f cycles\n", latency_diff);
printf("Signal strength: %.2f\n", signal_strength);
```

### 示例 3: 多参数测试
```c
for (int thread_config = 0; thread_config < 3; thread_config++) {
    for (int data_placement = 0; data_placement < 3; data_placement++) {
        cxl_config_t cfg;
        cxl_config_init(&cfg);
        cxl_set_thread_placement(&cfg, thread_config);
        cxl_set_data_placement(&cfg, data_placement);
        
        // 运行测试...
    }
}
```

## 系统要求与兼容性

### 硬件要求
- **处理器**: x86-64 架构，支持 RDTSCP、CLFLUSH 指令
- **NUMA**: 双节点 NUMA 系统 (节点 0: 普通内存, 节点 1: CXL Memory)
- **缓存**: L3 缓存（推荐 20 MB+）
- **内核**: Linux 5.0+

### 软件依赖
- GCC 7.0+
- libnuma-dev
- POSIX Threads
- 标准 C 库 (glibc)

### 测试系统配置 (参考)
- **CPU**: Intel Xeon (72 核心)
- **内存**: 512 GB (普通 + CXL)
- **内核**: Linux 5.15+
- **编译器**: GCC 11.2

## 框架的优势

1. **完整性**: 涵盖从初始化到分析的完整流程
2. **模块性**: 每个模块独立，便于扩展和维护
3. **灵活性**: 丰富的配置选项，适应不同测试场景
4. **易用性**: 清晰的 API 设计，提供了丰富的文档
5. **可扩展性**: 易于添加新的攻击方法或观测手段
6. **性能**: 优化的汇编实现和高效的算法

## 扩展方向

### 1. 添加新的攻击方法
在 `cxl_attack_primitives.h` 中定义新的原语，在对应的 .c 文件中实现。

### 2. 实现硬件性能计数器观测
集成 PAPI 或类似库来获取缓存命中等性能指标。

### 3. 添加机器学习模块
使用分类算法自动识别泄露的信息。

### 4. 实现实时可视化
集成图形库进行实时绘图。

### 5. 支持远程监控
添加网络通信功能进行分布式测试。

## 文档

该项目包含两份详细的文档：

1. **README.md** - 快速开始指南和使用说明
2. **API_REFERENCE.md** - 完整的 API 参考和函数文档

## 许可证

本项目采用 GPL v3.0 许可证。

## 贡献

欢迎提交 bug 报告和功能建议。

---

**项目版本**: 1.0  
**最后更新**: 2026 年 2 月  
**作者**: CXL Security Research Team

## 致谢

感谢参考框架 [enclyzer](https://github.com/bloaryth/enclyzer) 的设计灵感。
