#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include "cxl_attack_primitives.h"

/* ====== 静态阈值配置 ====== */
static uint64_t timing_threshold = 200;  /* 默认阈值 */

/* ====== 缓存清除原语 ====== */
void cxl_flush_clflush(void *addr) {
    asm volatile("clflush (%0)" : : "r" (addr) : "memory");
}

void cxl_flush_clflushopt(void *addr) {
    /* clflushopt 在某些 CPU 上不可用，使用条件编译 */
    #ifdef __clflushopt__
    asm volatile("clflushopt (%0)" : : "r" (addr) : "memory");
    #else
    cxl_flush_clflush(addr);  /* fallback to clflush */
    #endif
}

void cxl_flush_range(void *start_addr, void *end_addr) {
    if (!start_addr || !end_addr) return;
    
    uint64_t start = (uint64_t)start_addr;
    uint64_t end = (uint64_t)end_addr;
    
    /* 按缓存行大小进行 flush */
    for (uint64_t addr = start; addr < end; addr += CXL_CACHE_LINE_SIZE) {
        cxl_flush_clflush((void *)addr);
    }
}

/* ====== Probe 操作 ====== */
uint64_t cxl_probe_access_time(void *addr, uint64_t *out_time) {
    uint32_t cpu_id;
    volatile uint64_t *ptr = (volatile uint64_t *)addr;
    
    cxl_lfence();
    uint64_t start = cxl_rdtscp(&cpu_id);
    
    /* 执行内存访问 */
    (void)(*ptr);
    
    uint64_t end = cxl_rdtscp(&cpu_id);
    cxl_lfence();
    
    uint64_t access_time = end - start;
    
    if (out_time) {
        *out_time = access_time;
    }
    
    return access_time;
}

void cxl_probe_multiple(void **addrs, int num_addrs, uint64_t *timings) {
    if (!addrs || !timings || num_addrs <= 0) return;
    
    volatile uint64_t *ptr;
    uint32_t cpu_id;
    
    cxl_lfence();
    
    for (int i = 0; i < num_addrs; i++) {
        uint64_t start = cxl_rdtscp(&cpu_id);
        
        ptr = (volatile uint64_t *)addrs[i];
        (void)(*ptr);
        
        uint64_t end = cxl_rdtscp(&cpu_id);
        timings[i] = end - start;
    }
    
    cxl_lfence();
}

/* ====== Reload 操作 ====== */
void cxl_reload(void *addr) {
    volatile uint64_t *ptr = (volatile uint64_t *)addr;
    (void)(*ptr);
}

/* ====== 侧信道攻击操作 ====== */
uint64_t cxl_attack_load(void *addr) {
    uint64_t access_time = cxl_probe_access_time(addr, NULL);
    return access_time;
}

void cxl_attack_store(void *addr, uint64_t value) {
    volatile uint64_t *ptr = (volatile uint64_t *)addr;
    *ptr = value;
}

void cxl_attack_prefetch(void *addr) {
    /* 使用 prefetch 指令预加载数据，但不同 CPU 支持不同的变体 */
    asm volatile("prefetcht0 (%0)" : : "r" (addr));
}

void cxl_attack_mfence(void) {
    cxl_mfence();
}

void cxl_attack_lfence(void) {
    cxl_lfence();
}

/* ====== 受害者内存访问 ====== */
uint64_t cxl_victim_memory_access(void *addr, int is_write) {
    uint64_t access_time;
    
    if (is_write) {
        uint64_t start = cxl_rdtscp(NULL);
        volatile uint64_t *ptr = (volatile uint64_t *)addr;
        *ptr = 0x123456789ABCDEFULL;
        uint64_t end = cxl_rdtscp(NULL);
        access_time = end - start;
    } else {
        access_time = cxl_probe_access_time(addr, NULL);
    }
    
    return access_time;
}

/* ====== 随机内存访问 ====== */
void cxl_random_memory_access(void *start_addr, size_t size, int num_accesses) {
    if (!start_addr || size == 0 || num_accesses <= 0) return;
    
    volatile uint64_t *base = (volatile uint64_t *)start_addr;
    
    for (int i = 0; i < num_accesses; i++) {
        /* 生成最小的随机索引 */
        uint64_t offset = (rand() % (size / sizeof(uint64_t))) * sizeof(uint64_t);
        uint64_t idx = offset / sizeof(uint64_t);
        
        (void)(base[idx]);
    }
}

/* ====== 缓存驱逐 ====== */
void cxl_evict_cache_level(int level, void *evict_addr) {
    /* 驱逐对于特定缓存级级联较为复杂 */
    /* 这里提供一个简单的实现 */
    switch (level) {
        case 1:  /* L1 驱逐 */
            /* 通过多个访问来驱逐 L1 */
            for (int i = 0; i < 64; i++) {
                volatile uint64_t *ptr = (volatile uint64_t *)((uint64_t)evict_addr + i * 64);
                (void)(*ptr);
            }
            break;
        case 2:  /* L2 驱逐 */
            for (int i = 0; i < 256; i++) {
                volatile uint64_t *ptr = (volatile uint64_t *)((uint64_t)evict_addr + i * 64);
                (void)(*ptr);
            }
            break;
        case 3:  /* L3 驱逐 */
            for (int i = 0; i < 4096; i++) {
                volatile uint64_t *ptr = (volatile uint64_t *)((uint64_t)evict_addr + i * 64);
                (void)(*ptr);
            }
            break;
    }
}

/* ====== 组合攻击 ====== */
uint64_t cxl_flush_reload(void *addr) {
    cxl_flush_clflush(addr);
    cxl_mfence();
    
    /* 给 reload 留出时间 */
    for (volatile int i = 0; i < 100; i++) {}
    
    return cxl_probe_access_time(addr, NULL);
}

void cxl_flush_double(void *addr) {
    cxl_flush_clflush(addr);
    cxl_mfence();
    cxl_flush_clflush(addr);
    cxl_mfence();
}

uint64_t cxl_evict_time(void *evict_addr, void *probe_addr) {
    cxl_evict_cache_level(3, evict_addr);  /* 驱逐 L3 缓存 */
    cxl_mfence();
    
    return cxl_probe_access_time(probe_addr, NULL);
}

uint64_t cxl_spectre_variant(void *cond_addr, void *true_addr, void *probe_addr) {
    volatile uint32_t *cond = (volatile uint32_t *)cond_addr;
    volatile uint64_t *true_ptr = (volatile uint64_t *)true_addr;
    
    /* 造成推测执行 */
    if (*cond) {
        (void)(*true_ptr);
    }
    
    cxl_mfence();
    
    /* Probe 来检测侧信道 */
    return cxl_probe_access_time(probe_addr, NULL);
}

/* ====== 时间阈值管理 ====== */
void cxl_set_timing_threshold(uint64_t threshold) {
    timing_threshold = threshold;
    fprintf(stdout, "[INFO] Timing threshold set to %lu cycles\n", threshold);
}

uint64_t cxl_get_timing_threshold(void) {
    return timing_threshold;
}

/* ====== 原子操作 ====== */
void cxl_atomic_operation(void *addr) {
    volatile uint64_t *ptr = (volatile uint64_t *)addr;
    
    /* 使用原子加法进行同步 */
    asm volatile(
        "lock addq $1, (%0)"
        : : "r" (ptr) : "memory"
    );
}
