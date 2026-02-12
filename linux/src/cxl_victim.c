#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>
#include "cxl_victim.h"
#include "cxl_attack_primitives.h"
#include "cxl_attacker.h"
#include "cxl_common.h"

/* ====== 受害者状态管理 ====== */
static struct {
    int cpu_id;
    int initialized;
    victim_stats_t stats;
    int running;
    int command;
} victim_state = {0};

/* ====== 初始化与清理 ====== */
int cxl_victim_init(int cpu_id) {
    if (cxl_bind_to_cpu(cpu_id) < 0) {
        fprintf(stderr, "[ERROR] Failed to bind victim to CPU %d\n", cpu_id);
        return -1;
    }
    
    victim_state.cpu_id = cpu_id;
    victim_state.initialized = 1;
    memset(&victim_state.stats, 0, sizeof(victim_stats_t));
    
    fprintf(stdout, "[INFO] Victim initialized on CPU %d\n", cpu_id);
    
    return 0;
}

int cxl_victim_cleanup(void) {
    victim_state.initialized = 0;
    victim_state.running = 0;
    
    fprintf(stdout, "[INFO] Victim cleanup completed\n");
    
    return 0;
}

/* ====== 内存访问序列 ====== */
int cxl_victim_memory_sequence(void **addrs, int num_addrs, const char *access_pattern) {
    if (!addrs || num_addrs <= 0 || !access_pattern) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    if (!victim_state.initialized) {
        fprintf(stderr, "[ERROR] Victim not initialized\n");
        return -1;
    }
    
    volatile uint64_t *ptr;
    
    if (strcmp(access_pattern, "sequential") == 0) {
        /* 顺序访问 */
        for (int i = 0; i < num_addrs; i++) {
            ptr = (volatile uint64_t *)addrs[i];
            (void)(*ptr);
        }
    } else if (strcmp(access_pattern, "random") == 0) {
        /* 随机访问 */
        for (int i = 0; i < num_addrs; i++) {
            int idx = rand() % num_addrs;
            ptr = (volatile uint64_t *)addrs[idx];
            (void)(*ptr);
        }
    } else if (strcmp(access_pattern, "stride") == 0) {
        /* 步长访问 */
        for (int stride = 1; stride < num_addrs; stride *= 2) {
            for (int i = 0; i < num_addrs; i += stride) {
                ptr = (volatile uint64_t *)addrs[i];
                (void)(*ptr);
            }
        }
    } else {
        fprintf(stderr, "[WARNING] Unknown access pattern: %s\n", access_pattern);
        return -1;
    }
    
    return 0;
}

int cxl_victim_single_access(void *addr, int is_write) {
    if (!addr) {
        fprintf(stderr, "[ERROR] Invalid address\n");
        return -1;
    }
    
    if (!victim_state.initialized) {
        fprintf(stderr, "[ERROR] Victim not initialized\n");
        return -1;
    }
    
    return (int)cxl_victim_memory_access(addr, is_write);
}

/* ====== 工作负载 ====== */
int cxl_victim_workload(uint64_t duration_us, const char *workload_type) {
    if (duration_us == 0 || !workload_type) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    if (!victim_state.initialized) {
        fprintf(stderr, "[ERROR] Victim not initialized\n");
        return -1;
    }
    
    uint64_t cycles_per_us = 2400;  /* 假设 2.4 GHz CPU */
    uint64_t start_cycles = cxl_rdtscp(NULL);
    
    if (strcmp(workload_type, "cpu-intensive") == 0) {
        /* CPU 密集型工作负载 */
        volatile uint64_t result = 0;
        for (uint64_t i = 0; i < duration_us * cycles_per_us; i++) {
            result = (result * 1664525 + 1013904223) ^ i;
        }
    } else if (strcmp(workload_type, "memory-intensive") == 0) {
        /* 内存密集型工作负载 */
        size_t buffer_size = 1024 * 1024;  /* 1 MB */
        volatile uint8_t *buffer = malloc(buffer_size);
        
        uint64_t end_cycles = start_cycles + duration_us * cycles_per_us;
        while (cxl_rdtscp(NULL) < end_cycles) {
            for (size_t i = 0; i < buffer_size; i += 64) {
                (void)(buffer[i]);
            }
        }
        
        free((void *)buffer);
    } else {
        fprintf(stderr, "[WARNING] Unknown workload type: %s\n", workload_type);
        return -1;
    }
    
    return 0;
}

/* ====== 加密操作 ====== */
int cxl_victim_encrypt_operation(const void *key, const void *plaintext, void *ciphertext) {
    if (!key || !plaintext || !ciphertext) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    /* 简化的模拟加密操作 */
    const uint8_t *k = (const uint8_t *)key;
    const uint8_t *p = (const uint8_t *)plaintext;
    uint8_t *c = (uint8_t *)ciphertext;
    
    /* 执行一些内存访问以模拟加密 */
    for (int i = 0; i < 256; i++) {
        c[i] = p[i] ^ k[i % 32];
        
        /* 访问 S-Box 表（导致缓存侧信道） */
        volatile uint8_t sbox[256];
        for (int j = 0; j < 256; j++) {
            sbox[j] = (sbox[j] + c[i]) ^ k[j % 32];
        }
    }
    
    return 0;
}

/* ====== 查表操作 ====== */
int cxl_victim_lookup_operation(void *table, size_t table_size, 
                                const uint32_t *indices, int num_indices) {
    if (!table || table_size == 0 || !indices || num_indices <= 0) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    volatile uint8_t *t = (volatile uint8_t *)table;
    volatile uint8_t result = 0;
    
    for (int i = 0; i < num_indices; i++) {
        uint32_t idx = indices[i] % table_size;
        result ^= t[idx];  /* 访问表的不同位置 */
    }
    
    return (int)result;
}

/* ====== 分支操作 ====== */
int cxl_victim_branch_operation(int condition, void *true_data, void *false_data) {
    if (!true_data || !false_data) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    volatile uint64_t *data;
    
    /* 这导致推测执行 */
    if (condition) {
        data = (volatile uint64_t *)true_data;
    } else {
        data = (volatile uint64_t *)false_data;
    }
    
    /* 访问内存，留下缓存痕迹 */
    return (int)(*data);
}

/* ====== 链式访问 ====== */
int cxl_victim_chain_access(void **chain_addrs, int chain_size) {
    if (!chain_addrs || chain_size <= 0) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    volatile uint64_t *ptr = (volatile uint64_t *)chain_addrs[0];
    
    for (int i = 0; i < chain_size - 1; i++) {
        uint64_t next_offset = *ptr;
        ptr = (volatile uint64_t *)chain_addrs[i] + (next_offset % 256);
    }
    
    return (int)(*ptr);
}

/* ====== 空闲操作 ====== */
void cxl_victim_idle(uint64_t idle_duration_us) {
    /* 使用 nanosleep 实现精确的空闲 */
    struct timespec ts;
    ts.tv_sec = idle_duration_us / 1000000;
    ts.tv_nsec = (idle_duration_us % 1000000) * 1000;
    
    nanosleep(&ts, NULL);
}

/* ====== 统计信息 ====== */
int cxl_victim_get_statistics(victim_stats_t *stats) {
    if (!stats) {
        fprintf(stderr, "[ERROR] Invalid stats pointer\n");
        return -1;
    }
    
    memcpy(stats, &victim_state.stats, sizeof(victim_stats_t));
    
    return 0;
}

/* ====== 线程主循环 ====== */
void *cxl_victim_thread_main(void *arg) {
    int cpu_id = *(int *)arg;
    
    if (cxl_victim_init(cpu_id) < 0) {
        return NULL;
    }
    
    victim_state.running = 1;
    
    while (victim_state.running) {
        /* 等待命令 */
        int cmd = cxl_victim_wait_for_command();
        
        if (cmd < 0) break;
        
        switch (cmd) {
            case 0:  /* NOP */
                break;
            case 1:  /* 内存访问 */
                {
                    void *addr = (void *)(uintptr_t)arg;
                    cxl_victim_single_access(addr, 0);
                }
                break;
            default:
                fprintf(stderr, "[WARNING] Unknown victim command: %d\n", cmd);
        }
    }
    
    cxl_victim_cleanup();
    
    return NULL;
}

/* ====== 命令等待 ====== */
int cxl_victim_wait_for_command(void) {
    /* 这是一个简化的实现，实际应该使用信号量或条件变量 */
    int cmd = victim_state.command;
    victim_state.command = 0;
    
    return cmd;
}

/* ====== 定时循环 ====== */
int cxl_victim_timed_loop(void *addr, uint64_t iterations, uint64_t interval_us) {
    if (!addr || iterations == 0) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = interval_us * 1000;
    
    for (uint64_t i = 0; i < iterations; i++) {
        cxl_victim_single_access(addr, 0);
        nanosleep(&ts, NULL);
    }
    
    return 0;
}
