#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include "cxl_observation.h"
#include "cxl_common.h"

/* ====== 观测缓冲区管理 ====== */
static struct {
    observation_type_t type;
    void *buffer;
    size_t buffer_size;
    size_t current_pos;
    int initialized;
    pthread_t monitor_thread;
    int monitor_running;
    void *monitor_target;
    observation_callback_t monitor_callback;
    void *monitor_context;
} observation_state = {0};

/* ====== 初始化与清理 ====== */
int cxl_observation_init(observation_type_t observation_type, size_t buffer_size) {
    if (buffer_size == 0) {
        fprintf(stderr, "[ERROR] Invalid buffer size\n");
        return -1;
    }
    
    observation_state.type = observation_type;
    observation_state.buffer = malloc(buffer_size);
    
    if (!observation_state.buffer) {
        fprintf(stderr, "[ERROR] Failed to allocate observation buffer\n");
        return -1;
    }
    
    observation_state.buffer_size = buffer_size;
    observation_state.current_pos = 0;
    observation_state.initialized = 1;
    observation_state.monitor_running = 0;
    
    fprintf(stdout, "[INFO] Observation module initialized\n");
    
    return 0;
}

int cxl_observation_cleanup(void) {
    if (observation_state.monitor_running) {
        cxl_observe_realtime_stop();
    }
    
    if (observation_state.buffer) {
        free(observation_state.buffer);
        observation_state.buffer = NULL;
    }
    
    observation_state.initialized = 0;
    
    fprintf(stdout, "[INFO] Observation module cleanup completed\n");
    
    return 0;
}

/* ====== 访问时间观测 ====== */
int cxl_observe_access_timing(void *addr, int num_samples, uint64_t *samples) {
    if (!addr || !samples || num_samples <= 0) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    if (!observation_state.initialized) {
        fprintf(stderr, "[ERROR] Observation module not initialized\n");
        return -1;
    }
    
    volatile uint64_t *ptr = (volatile uint64_t *)addr;
    
    cxl_lfence();
    
    for (int i = 0; i < num_samples; i++) {
        uint32_t cpu_id;
        uint64_t start = cxl_rdtscp(&cpu_id);
        
        (void)(*ptr);
        
        uint64_t end = cxl_rdtscp(&cpu_id);
        samples[i] = end - start;
        
        /* 小延迟避免连续缓存命中 */
        for (volatile int j = 0; j < 100; j++) {}
    }
    
    cxl_lfence();
    
    return num_samples;
}

/* ====== 缓存模式观测 ====== */
int cxl_observe_cache_pattern(void **addrs, int num_addrs, int num_probes, 
                              uint8_t *hit_patterns) {
    if (!addrs || num_addrs <= 0 || !hit_patterns || num_probes <= 0) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    if (!observation_state.initialized) {
        fprintf(stderr, "[ERROR] Observation module not initialized\n");
        return -1;
    }
    
    uint64_t threshold = cxl_get_timing_threshold();
    int pattern_idx = 0;
    
    for (int addr_idx = 0; addr_idx < num_addrs; addr_idx++) {
        for (int probe = 0; probe < num_probes; probe++) {
            uint64_t access_time = cxl_probe_access_time(addrs[addr_idx], NULL);
            
            hit_patterns[pattern_idx] = (access_time < threshold) ? 1 : 0;
            pattern_idx++;
            
            /* Flush 以进行下一次探测 */
            cxl_flush_clflush(addrs[addr_idx]);
            cxl_mfence();
        }
    }
    
    return pattern_idx;
}

/* ====== 访问痕迹观测 ====== */
int cxl_observe_access_trace(void *addr_range_start, size_t addr_range_size,
                             int *num_traces, uint32_t *traces) {
    if (!addr_range_start || addr_range_size == 0 || !traces) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    /* 这是一个简化的实现 */
    /* 实际的访问痕迹需要硬件支持（如 Intel PT 或类似机制） */
    
    *num_traces = 0;
    
    return 0;
}

/* ====== RDTSCP 样本采集 ====== */
int cxl_observe_rdtscp_samples(timing_sample_t *samples, int num_samples) {
    if (!samples || num_samples <= 0) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    if (!observation_state.initialized) {
        fprintf(stderr, "[ERROR] Observation module not initialized\n");
        return -1;
    }
    
    cxl_lfence();
    
    for (int i = 0; i < num_samples; i++) {
        uint32_t apic_id;
        samples[i].tsc = cxl_rdtscp(&apic_id);
        samples[i].apic_id = apic_id;
        samples[i].timestamp = samples[i].tsc;  /* 简化，实际应转换为纳秒 */
        
        /* 微小延迟 */
        for (volatile int j = 0; j < 10; j++) {}
    }
    
    cxl_lfence();
    
    return num_samples;
}

/* ====== 访问间隔观测 ====== */
int cxl_observe_access_intervals(void *addr, int max_intervals, uint64_t *intervals) {
    if (!addr || !intervals || max_intervals <= 0) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    volatile uint64_t *ptr = (volatile uint64_t *)addr;
    uint64_t prev_time = cxl_rdtscp(NULL);
    int count = 0;
    
    for (int i = 0; i < max_intervals; i++) {
        for (volatile int j = 0; j < 1000; j++) {}  /* 有意延迟 */
        
        uint64_t curr_time = cxl_rdtscp(NULL);
        (void)(*ptr);  /* 访问内存 */
        uint64_t next_time = cxl_rdtscp(NULL);
        
        intervals[count] = next_time - curr_time;
        count++;
        
        prev_time = next_time;
    }
    
    return count;
}

/* ====== 时间重叠分析 ====== */
int cxl_observe_timing_overlap(uint64_t attacker_timing, uint64_t victim_timing,
                               uint64_t *overlap_time) {
    if (!overlap_time) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    /* 简化的重叠检测 */
    uint64_t time_diff = (attacker_timing > victim_timing) ? 
                         (attacker_timing - victim_timing) : 
                         (victim_timing - attacker_timing);
    
    uint64_t threshold = 10000;  /* 10000 cycles 作为重叠阈值 */
    
    if (time_diff < threshold) {
        *overlap_time = threshold - time_diff;
        return 0;  /* 有重叠 */
    }
    
    return 1;  /* 无重叠 */
}

/* ====== 缓存冲突观测 ====== */
int cxl_observe_cache_conflicts(void **addrs, int num_addrs, uint8_t *conflicts) {
    if (!addrs || num_addrs <= 0 || !conflicts) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    /* 简单的冲突检测：相同 L3 索引会引起冲突 */
    for (int i = 0; i < num_addrs; i++) {
        for (int j = i + 1; j < num_addrs; j++) {
            uint64_t addr_i = (uint64_t)addrs[i];
            uint64_t addr_j = (uint64_t)addrs[j];
            
            /* 检查是否映射到相同的 L3 集合 */
            uint64_t idx_i = (addr_i >> 6) & 0xFFF;  /* L3 索引 */
            uint64_t idx_j = (addr_j >> 6) & 0xFFF;
            
            if (idx_i == idx_j) {
                conflicts[i] |= (1 << (j % 8));
                conflicts[j] |= (1 << (i % 8));
            }
        }
    }
    
    return 0;
}

/* ====== L3 缓存时间观测 ====== */
int cxl_observe_l3_timing(void *addr, int num_measurements, uint64_t *timings) {
    if (!addr || num_measurements <= 0 || !timings) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    for (int i = 0; i < num_measurements; i++) {
        /* Flush L3 */
        cxl_flush_clflush(addr);
        cxl_mfence();
        
        /* 长延迟 */
        for (volatile int j = 0; j < 100000; j++) {}
        
        /* Reload 测时 */
        timings[i] = cxl_probe_access_time(addr, NULL);
    }
    
    return num_measurements;
}

/* ====== CXL Memory 延迟观测 ====== */
int cxl_observe_cxl_latency(void *cxl_addr, void *normal_addr,
                            uint64_t *cxl_timings, uint64_t *normal_timings,
                            int num_samples) {
    if (!cxl_addr || !normal_addr || !cxl_timings || !normal_timings || num_samples <= 0) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    for (int i = 0; i < num_samples; i++) {
        cxl_timings[i] = cxl_probe_access_time(cxl_addr, NULL);
        normal_timings[i] = cxl_probe_access_time(normal_addr, NULL);
        
        /* 清除缓存以隔离测量 */
        cxl_flush_clflush(cxl_addr);
        cxl_flush_clflush(normal_addr);
        cxl_mfence();
    }
    
    return num_samples;
}

/* ====== 统计分析 ====== */
int cxl_observe_statistics(const uint64_t *samples, int num_samples,
                           uint64_t *min, uint64_t *max, 
                           double *mean, double *stddev) {
    if (!samples || num_samples <= 0 || !min || !max || !mean || !stddev) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    *min = samples[0];
    *max = samples[0];
    *mean = 0.0;
    
    /* 计算最小值、最大值和平均值 */
    for (int i = 0; i < num_samples; i++) {
        if (samples[i] < *min) *min = samples[i];
        if (samples[i] > *max) *max = samples[i];
        *mean += samples[i];
    }
    *mean /= num_samples;
    
    /* 计算标准差 */
    double variance = 0.0;
    for (int i = 0; i < num_samples; i++) {
        double diff = samples[i] - *mean;
        variance += diff * diff;
    }
    variance /= num_samples;
    *stddev = sqrt(variance);
    
    return 0;
}

/* ====== 异常检测 ====== */
int cxl_observe_detect_anomalies(const uint64_t *samples, int num_samples,
                                 double threshold, uint8_t *anomalies) {
    if (!samples || num_samples <= 0 || !anomalies) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    uint64_t min, max;
    double mean, stddev;
    
    if (cxl_observe_statistics(samples, num_samples, &min, &max, &mean, &stddev) < 0) {
        return -1;
    }
    
    int anomaly_count = 0;
    
    for (int i = 0; i < num_samples; i++) {
        double z_score = (samples[i] - mean) / (stddev + 1e-9);
        
        if (fabs(z_score) > threshold) {
            anomalies[i / 8] |= (1 << (i % 8));
            anomaly_count++;
        } else {
            anomalies[i / 8] &= ~(1 << (i % 8));
        }
    }
    
    return anomaly_count;
}

/* ====== 实时观测线程 ====== */
static void *observation_monitor_thread(void *arg) {
    while (observation_state.monitor_running) {
        observation_data_t data;
        
        data.sample_id++;
        data.timestamp = cxl_rdtscp(&data.cpu_id);
        data.access_time = cxl_probe_access_time(observation_state.monitor_target, NULL);
        data.address = (uint64_t)observation_state.monitor_target;
        
        uint64_t threshold = cxl_get_timing_threshold();
        data.is_hit = (data.access_time < threshold) ? 1 : 0;
        
        if (observation_state.monitor_callback) {
            observation_state.monitor_callback(&data, observation_state.monitor_context);
        }
        
        usleep(1000);  /* 每 1ms 采样一次 */
    }
    
    return NULL;
}

int cxl_observe_realtime_start(void *target_addr, observation_type_t observation_type,
                               observation_callback_t callback, void *ctx) {
    if (!target_addr || !callback) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    if (observation_state.monitor_running) {
        fprintf(stderr, "[WARNING] Realtime monitor already running\n");
        return -1;
    }
    
    observation_state.monitor_target = target_addr;
    observation_state.monitor_callback = callback;
    observation_state.monitor_context = ctx;
    observation_state.monitor_running = 1;
    
    if (pthread_create(&observation_state.monitor_thread, NULL, 
                      observation_monitor_thread, NULL) < 0) {
        fprintf(stderr, "[ERROR] Failed to create monitor thread\n");
        observation_state.monitor_running = 0;
        return -1;
    }
    
    fprintf(stdout, "[INFO] Realtime observation started\n");
    
    return 0;
}

int cxl_observe_realtime_stop(void) {
    if (!observation_state.monitor_running) {
        return -1;
    }
    
    observation_state.monitor_running = 0;
    pthread_join(observation_state.monitor_thread, NULL);
    
    fprintf(stdout, "[INFO] Realtime observation stopped\n");
    
    return 0;
}

/* ====== 缓冲区管理 ====== */
int cxl_observation_clear_buffer(void) {
    if (!observation_state.initialized) {
        return -1;
    }
    
    memset(observation_state.buffer, 0, observation_state.buffer_size);
    observation_state.current_pos = 0;
    
    return 0;
}

int cxl_observation_get_data(void *buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0 || !observation_state.initialized) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    size_t copy_size = (buffer_size < observation_state.current_pos) ? 
                       buffer_size : observation_state.current_pos;
    
    memcpy(buffer, observation_state.buffer, copy_size);
    
    return copy_size;
}
