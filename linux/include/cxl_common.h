#ifndef CXL_COMMON_H
#define CXL_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <sched.h>

/* ====== 常量定义 ====== */
#define CXL_MAX_CORES           256
#define CXL_MAX_NODES           8
#define CXL_CACHE_LINE_SIZE     64
#define CXL_PAGE_SIZE           4096
#define CXL_MAX_THREADS         64
#define CXL_RESULT_BUFFER_SIZE  1000000

/* ====== NUMA 节点配置 ====== */
#define NUMA_NODE_NORMAL        0
#define NUMA_NODE_CXL_MEMORY    1

/* ====== 攻击者/受害者位置配置 ====== */
typedef enum {
    CROSS_CORE,          /* 不同核心 */
    DIFFERENT_THREAD,    /* 不同线程 */
    SAME_THREAD          /* 同线程 */
} thread_placement_t;

/* ====== 数据放置配置 ====== */
typedef enum {
    PLACEMENT_NORMAL_NODE,     /* 普通 NUMA 节点 */
    PLACEMENT_CXL_MEMORY,      /* CXL 内存 */
    PLACEMENT_LOCAL            /* 本地 CPU 缓存 */
} data_placement_t;

/* ====== 侧信道观测类型 ====== */
typedef enum {
    OBSERVE_TIMING,     /* 访问时间观测 */
    OBSERVE_PATTERN,    /* 访问模式观测 */
    OBSERVE_TRACE       /* 访问痕迹观测 */
} observation_type_t;

/* ====== 时间戳结构 ====== */
typedef struct {
    uint64_t tsc;           /* TSC 值 */
    uint64_t apic_id;       /* APIC 核心 ID */
    uint64_t timestamp;     /* 纳秒时间戳 */
} timing_sample_t;

/* ====== 攻击结果结构 ====== */
typedef struct {
    uint64_t attack_id;
    uint64_t victim_access_time;
    uint64_t attacker_probe_time;
    uint64_t latency_diff;
    uint32_t hit_count;     /* 命中次数 */
    uint32_t miss_count;    /* 未命中次数 */
    uint8_t is_hit;         /* 是否命中 */
    data_placement_t data_location;
    thread_placement_t thread_config;
} attack_result_t;

/* ====== 观测数据结构 ====== */
typedef struct {
    uint64_t sample_id;
    uint64_t timestamp;
    uint64_t access_time;
    uint32_t cpu_id;
    uint8_t is_hit;
    uint64_t address;
} observation_data_t;

/* ====== 工作线程信息 ====== */
typedef struct {
    int thread_id;
    int cpu_id;
    int node_id;
    pthread_t pthread_id;
} thread_info_t;

/* ====== 框架配置结构 ====== */
typedef struct {
    /* NUMA 配置 */
    int numa_node_normal;
    int numa_node_cxl;
    
    /* 线程位置配置 */
    thread_placement_t thread_placement;
    
    /* 数据放置配置 */
    data_placement_t data_placement;
    
    /* CPU 配置 */
    int attacker_cpu;
    int victim_cpu;
    int probe_cpu;
    int monitor_cpu;
    
    /* 系统配置 */
    int prefetcher_enabled;
    int isolcpus_enabled;
    
    /* 工作参数 */
    uint64_t iterations;
    uint64_t warmup_iterations;
    uint32_t sample_size;
} cxl_config_t;

/* ====== 内联函数：快速获取 RDTSCP 时间戳 ====== */
static inline uint64_t cxl_rdtscp(uint32_t *cpu_id) {
    uint32_t eax, edx, ecx;
    
    asm volatile(
        "rdtscp"
        : "=a" (eax), "=d" (edx), "=c" (ecx)
        : : "memory"
    );
    
    if (cpu_id) {
        *cpu_id = ecx & 0xFFF;
    }
    
    return ((uint64_t)edx << 32) | eax;
}

/* ====== 内联函数：轻量级内存屏障 ====== */
static inline void cxl_mfence(void) {
    asm volatile("mfence" : : : "memory");
}

/* ====== 内联函数：序列化指令 ====== */
static inline void cxl_lfence(void) {
    asm volatile("lfence" : : : "memory");
}

/* ====== 内联函数：通用序列化点 ====== */
static inline void cxl_serialization_point(void) {
    cxl_mfence();
}

/* ====== 内联函数：计算访问时间 ====== */
static inline uint64_t cxl_access_time(uint64_t start, uint64_t end) {
    return (end > start) ? (end - start) : 0;
}

/* ====== CXL 内存地址辅助函数 ====== */
void *cxl_malloc_on_node(size_t size, int node);
void cxl_free(void *ptr, size_t size);
int cxl_bind_to_cpu(int cpu_id);
int cxl_bind_to_node(int node_id);

/* ====== 日志函数 ====== */
void cxl_log_info(const char *format, ...);
void cxl_log_error(const char *format, ...);
void cxl_log_warning(const char *format, ...);

#endif /* CXL_COMMON_H */
