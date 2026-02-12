#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "cxl_common.h"
#include "cxl_prepreparation.h"
#include "cxl_attack_primitives.h"
#include "cxl_victim.h"
#include "cxl_attacker.h"
#include "cxl_observation.h"
#include "cxl_analysis.h"

/* ====== 全局框架状态 ====== */
static struct {
    cxl_config_t config;
    int initialized;
    pthread_t victim_thread;
    pthread_t attacker_thread;
    int num_threads;
} framework_state = {0};

/* ====== 框架初始化 ====== */
int cxl_framework_init(const char *config_name) {
    fprintf(stdout, "\n========== CXL Side-Channel Detection Framework ==========\n");
    fprintf(stdout, "Version 1.0 - Linux CXL Memory Security Analysis\n");
    fprintf(stdout, "=========================================================\n\n");
    
    /* 初始化配置 */
    if (cxl_config_init(&framework_state.config) < 0) {
        fprintf(stderr, "[ERROR] Failed to initialize configuration\n");
        return -1;
    }
    
    /* 检查系统支持 */
    if (cxl_check_system_support() < 1) {
        fprintf(stderr, "[WARNING] CXL system support check returned warning\n");
    }
    
    /* 验证配置 */
    if (cxl_validate_config(&framework_state.config) < 0) {
        fprintf(stderr, "[ERROR] Configuration validation failed\n");
        return -1;
    }
    
    framework_state.initialized = 1;
    
    fprintf(stdout, "[INFO] Framework initialized successfully\n\n");
    
    return 0;
}

/* ====== 框架清理 ====== */
int cxl_framework_cleanup(void) {
    framework_state.initialized = 0;
    
    fprintf(stdout, "[INFO] Framework cleanup completed\n");
    
    return 0;
}

/* ====== 获取框架配置 ====== */
cxl_config_t *cxl_framework_get_config(void) {
    if (!framework_state.initialized) {
        fprintf(stderr, "[ERROR] Framework not initialized\n");
        return NULL;
    }
    
    return &framework_state.config;
}

/* ====== 打印框架信息 ====== */
void cxl_framework_print_info(void) {
    fprintf(stdout, "\n========== CXL Framework Information ==========\n");
    
    int num_cpus = cxl_get_num_cpus();
    int num_nodes = cxl_get_num_numa_nodes();
    
    fprintf(stdout, "System Information:\n");
    fprintf(stdout, "  Total CPUs:        %d\n", num_cpus);
    fprintf(stdout, "  NUMA Nodes:        %d\n", num_nodes);
    fprintf(stdout, "  Cache Line Size:   %d bytes\n", CXL_CACHE_LINE_SIZE);
    fprintf(stdout, "  CXL Node:          %d\n", cxl_get_cxl_node());
    
    fprintf(stdout, "\nCurrent Configuration:\n");
    cxl_print_config(&framework_state.config);
}

/* ====== 执行 Flush + Reload 攻击 ====== */
int cxl_framework_test_flush_reload(void *victim_data, int num_iterations) {
    if (!victim_data) {
        fprintf(stderr, "[ERROR] Invalid victim data pointer\n");
        return -1;
    }
    
    fprintf(stdout, "\n[TEST] Starting Flush + Reload Attack\n");
    fprintf(stdout, "  Target: %p\n", victim_data);
    fprintf(stdout, "  Iterations: %d\n\n", num_iterations);
    
    attack_result_t *results = malloc(num_iterations * sizeof(attack_result_t));
    if (!results) {
        fprintf(stderr, "[ERROR] Failed to allocate results buffer\n");
        return -1;
    }
    
    for (int i = 0; i < num_iterations; i++) {
        if (cxl_attacker_flush_reload(victim_data, &results[i]) < 0) {
            fprintf(stderr, "[WARNING] Attack iteration %d failed\n", i);
            continue;
        }
        
        if (i % 100 == 0 && i > 0) {
            fprintf(stdout, "  Completed: %d/%d iterations\n", i, num_iterations);
        }
    }
    
    /* 分析结果 */
    float success_rate = cxl_attacker_success_rate(results, num_iterations);
    
    fprintf(stdout, "\n[RESULT] Flush + Reload Attack Summary\n");
    fprintf(stdout, "  Success Rate: %.2f%%\n", success_rate * 100.0f);
    fprintf(stdout, "  Total Hits: %d/%d\n", 
           (int)(success_rate * num_iterations), num_iterations);
    
    free(results);
    
    return 0;
}

/* ====== 执行 CXL Memory 延迟测试 ====== */
int cxl_framework_test_cxl_latency(void *cxl_addr, void *normal_addr, int num_samples) {
    if (!cxl_addr || !normal_addr) {
        fprintf(stderr, "[ERROR] Invalid memory addresses\n");
        return -1;
    }
    
    fprintf(stdout, "\n[TEST] Starting CXL Memory Latency Test\n");
    fprintf(stdout, "  CXL Address:     %p\n", cxl_addr);
    fprintf(stdout, "  Normal Address:  %p\n", normal_addr);
    fprintf(stdout, "  Samples:         %d\n\n", num_samples);
    
    uint64_t *cxl_timings = malloc(num_samples * sizeof(uint64_t));
    uint64_t *normal_timings = malloc(num_samples * sizeof(uint64_t));
    
    if (!cxl_timings || !normal_timings) {
        fprintf(stderr, "[ERROR] Failed to allocate timing buffers\n");
        return -1;
    }
    
    if (cxl_observe_cxl_latency(cxl_addr, normal_addr, cxl_timings, normal_timings, num_samples) < 0) {
        fprintf(stderr, "[ERROR] Latency observation failed\n");
        free(cxl_timings);
        free(normal_timings);
        return -1;
    }
    
    /* 分析延迟差异 */
    double latency_diff = 0.0, signal_strength = 0.0;
    cxl_analysis_latency_difference(cxl_timings, normal_timings, num_samples, 
                                    &latency_diff, &signal_strength);
    
    /* 计算统计信息 */
    uint64_t cxl_min, cxl_max, normal_min, normal_max;
    double cxl_mean, cxl_median, cxl_stddev;
    double normal_mean, normal_median, normal_stddev;
    
    cxl_analysis_compute_statistics(cxl_timings, num_samples, 
                                    &cxl_min, &cxl_max, &cxl_mean, 
                                    &cxl_median, &cxl_stddev);
    
    cxl_analysis_compute_statistics(normal_timings, num_samples, 
                                    &normal_min, &normal_max, &normal_mean, 
                                    &normal_median, &normal_stddev);
    
    fprintf(stdout, "[RESULT] CXL Memory Latency Test Summary\n");
    fprintf(stdout, "\nCXL Memory Latency:\n");
    fprintf(stdout, "  Min:      %lu cycles\n", cxl_min);
    fprintf(stdout, "  Max:      %lu cycles\n", cxl_max);
    fprintf(stdout, "  Mean:     %.2f cycles\n", cxl_mean);
    fprintf(stdout, "  Median:   %.2f cycles\n", cxl_median);
    fprintf(stdout, "  StdDev:   %.2f cycles\n", cxl_stddev);
    
    fprintf(stdout, "\nNormal Memory Latency:\n");
    fprintf(stdout, "  Min:      %lu cycles\n", normal_min);
    fprintf(stdout, "  Max:      %lu cycles\n", normal_max);
    fprintf(stdout, "  Mean:     %.2f cycles\n", normal_mean);
    fprintf(stdout, "  Median:   %.2f cycles\n", normal_median);
    fprintf(stdout, "  StdDev:   %.2f cycles\n", normal_stddev);
    
    fprintf(stdout, "\nLatency Difference Analysis:\n");
    fprintf(stdout, "  Latency Difference:  %.2f cycles\n", latency_diff);
    fprintf(stdout, "  Signal Strength:     %.2f\n", signal_strength);
    
    free(cxl_timings);
    free(normal_timings);
    
    return 0;
}

/* ====== 执行多线程测试 ====== */
int cxl_framework_test_multithreading(int num_threads) {
    if (num_threads <= 0) {
        fprintf(stderr, "[ERROR] Invalid number of threads\n");
        return -1;
    }
    
    fprintf(stdout, "\n[TEST] Starting Multi-threading Test\n");
    fprintf(stdout, "  Threads: %d\n\n", num_threads);
    
    if (cxl_setup_multithreading(&framework_state.config, num_threads) < 0) {
        fprintf(stderr, "[ERROR] Failed to setup multithreading\n");
        return -1;
    }
    
    fprintf(stdout, "[INFO] Multi-threading test completed\n");
    
    return 0;
}

/* ====== 执行单线程隔离测试 ====== */
int cxl_framework_test_singlethreading_isolated(void) {
    fprintf(stdout, "\n[TEST] Starting Single-thread Isolation Test\n\n");
    
    if (cxl_setup_singlethreading(&framework_state.config) < 0) {
        fprintf(stderr, "[ERROR] Failed to setup single threading\n");
        return -1;
    }
    
    fprintf(stdout, "[INFO] Single-thread isolation test completed\n");
    
    return 0;
}

/* ====== 完整的攻击演示 ====== */
int cxl_framework_run_full_demo(void *test_data, const char *output_dir) {
    if (!test_data || !output_dir) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    fprintf(stdout, "\n========== Running Full CXL Security Demonstration ==========\n\n");
    
    /* 初始化分析模块 */
    if (cxl_analysis_init(output_dir) < 0) {
        fprintf(stderr, "[ERROR] Failed to initialize analysis module\n");
        return -1;
    }
    
    /* 初始化观测模块 */
    if (cxl_observation_init(OBSERVE_TIMING, 1000000) < 0) {
        fprintf(stderr, "[ERROR] Failed to initialize observation module\n");
        return -1;
    }
    
    /* 运行 Flush + Reload 测试 */
    if (cxl_framework_test_flush_reload(test_data, 1000) < 0) {
        fprintf(stderr, "[ERROR] Flush + Reload test failed\n");
    }
    
    /* 清理 */
    cxl_observation_cleanup();
    cxl_analysis_cleanup();
    
    fprintf(stdout, "\n[INFO] Full demonstration completed\n");
    fprintf(stdout, "Results saved to: %s\n", output_dir);
    
    return 0;
}

/* ====== 测试配置结构 ====== */
typedef struct {
    int test_mode;                  /* 测试模式 */
    int num_iterations;             /* 迭代次数 */
    int num_rounds;                 /* 轮次 */
    int num_threads;                /* 线程数 */
    char output_dir[256];           /* 输出目录 */
    int compare_cxl_normal;         /* 是否对比 CXL vs Normal */
    int enable_stats;               /* 是否启用统计 */
    int verbose;                    /* 详细输出 */
} test_config_t;

/* ====== 打印帮助信息 ====== */
void print_usage(const char *prog_name) {
    fprintf(stdout, "\nUsage: %s [OPTIONS]\n\n", prog_name);
    fprintf(stdout, "Test Modes:\n");
    fprintf(stdout, "  -m 0   : Flush + Reload Attack (default)\n");
    fprintf(stdout, "  -m 1   : CXL Memory Latency Test\n");
    fprintf(stdout, "  -m 2   : Multi-threading Test\n");
    fprintf(stdout, "  -m 3   : Single-thread Isolation Test\n");
    fprintf(stdout, "  -m 4   : Full Demonstration (All Tests)\n");
    
    fprintf(stdout, "\nOptions:\n");
    fprintf(stdout, "  -i ITER    : Number of iterations (default: 1000)\n");
    fprintf(stdout, "  -r ROUNDS  : Number of rounds for statistics (default: 5)\n");
    fprintf(stdout, "  -t THREADS : Number of threads (default: 4)\n");
    fprintf(stdout, "  -o OUTDIR  : Output directory (default: ./results)\n");
    fprintf(stdout, "  -c         : Compare CXL vs Normal memory\n");
    fprintf(stdout, "  -s         : Enable detailed statistics\n");
    fprintf(stdout, "  -v         : Verbose output\n");
    fprintf(stdout, "  -h         : Print this help\n\n");
}

/* ====== 解析命令行参数 ====== */
int parse_args(int argc, char *argv[], test_config_t *config) {
    /* 默认配置 */
    config->test_mode = 0;
    config->num_iterations = 1000;
    config->num_rounds = 5;
    config->num_threads = 4;
    config->compare_cxl_normal = 0;
    config->enable_stats = 0;
    config->verbose = 0;
    strncpy(config->output_dir, "./results", sizeof(config->output_dir) - 1);
    
    /* 解析参数 */
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') continue;
        
        switch (argv[i][1]) {
            case 'm':
                if (i + 1 < argc) config->test_mode = atoi(argv[++i]);
                break;
            case 'i':
                if (i + 1 < argc) config->num_iterations = atoi(argv[++i]);
                break;
            case 'r':
                if (i + 1 < argc) config->num_rounds = atoi(argv[++i]);
                break;
            case 't':
                if (i + 1 < argc) config->num_threads = atoi(argv[++i]);
                break;
            case 'o':
                if (i + 1 < argc) strncpy(config->output_dir, argv[++i], 
                                         sizeof(config->output_dir) - 1);
                break;
            case 'c':
                config->compare_cxl_normal = 1;
                break;
            case 's':
                config->enable_stats = 1;
                break;
            case 'v':
                config->verbose = 1;
                break;
            case 'h':
                print_usage(argv[0]);
                return 1;
            default:
                fprintf(stderr, "[ERROR] Unknown option: %s\n", argv[i]);
                print_usage(argv[0]);
                return -1;
        }
    }
    
    return 0;
}

/* ====== 执行 Flush+Reload 完整测试 ====== */
int run_flush_reload_test(test_config_t *config) {
    fprintf(stdout, "\n============== Flush + Reload Attack Test ==============\n");
    fprintf(stdout, "Iterations: %d, Rounds: %d\n\n", 
           config->num_iterations, config->num_rounds);
    
    double total_success_rate = 0.0;
    uint64_t min_success = UINT64_MAX, max_success = 0;
    
    /* 多轮测试 */
    for (int round = 0; round < config->num_rounds; round++) {
        fprintf(stdout, "[Round %d/%d]", round + 1, config->num_rounds);
        
        size_t test_size = 4096;
        void *test_data = cxl_malloc_on_node(test_size, 0);
        
        if (!test_data) {
            fprintf(stderr, " [ERROR] Failed to allocate test data\n");
            continue;
        }
        
        attack_result_t *results = malloc(config->num_iterations * 
                                         sizeof(attack_result_t));
        if (!results) {
            fprintf(stderr, " [ERROR] Failed to allocate results\n");
            cxl_free(test_data, test_size);
            continue;
        }
        
        /* 执行攻击 */
        int success_count = 0;
        for (int i = 0; i < config->num_iterations; i++) {
            if (cxl_attacker_flush_reload(test_data, &results[i]) >= 0 && 
                results[i].is_hit) {
                success_count++;
            }
            
            if (config->verbose && i % 100 == 0 && i > 0) {
                fprintf(stdout, ".");
            }
        }
        
        double success_rate = (double)success_count / config->num_iterations;
        total_success_rate += success_rate;
        min_success = (success_count < min_success) ? success_count : min_success;
        max_success = (success_count > max_success) ? success_count : max_success;
        
        fprintf(stdout, " Success Rate: %.2f%% (%d/%d hits)\n", 
               success_rate * 100.0, success_count, config->num_iterations);
        
        free(results);
        cxl_free(test_data, test_size);
    }
    
    /* 汇总统计 */
    fprintf(stdout, "\n[SUMMARY] Flush + Reload Test Results:\n");
    fprintf(stdout, "  Average Success Rate: %.2f%%\n", 
           (total_success_rate / config->num_rounds) * 100.0);
    fprintf(stdout, "  Min Hits per Round:   %lu\n", min_success);
    fprintf(stdout, "  Max Hits per Round:   %lu\n", max_success);
    
    return 0;
}

/* ====== 执行延迟测试与对比 ====== */
int run_latency_test(test_config_t *config) {
    fprintf(stdout, "\n============== CXL Memory Latency Test ==============\n");
    fprintf(stdout, "Samples: %d per access, Rounds: %d\n", 
           config->num_iterations, config->num_rounds);
    
    if (!config->compare_cxl_normal) {
        fprintf(stdout, "[Note] Run with -c flag to compare CXL vs Normal memory\n\n");
    }
    
    double total_diff = 0.0;
    double total_signal = 0.0;
    
    /* 多轮测试 */
    for (int round = 0; round < config->num_rounds; round++) {
        fprintf(stdout, "\n[Round %d/%d]\n", round + 1, config->num_rounds);
        
        /* 在 Normal Node 上分配数据 */
        size_t test_size = 4096;
        void *normal_addr = cxl_malloc_on_node(NUMA_NODE_NORMAL, test_size);
        void *cxl_addr = config->compare_cxl_normal ? 
                        cxl_malloc_on_node(NUMA_NODE_CXL_MEMORY, test_size) :
                        normal_addr;
        
        if (!normal_addr || !cxl_addr) {
            fprintf(stderr, "[ERROR] Memory allocation failed\n");
            continue;
        }
        
        uint64_t *cxl_timings = malloc(config->num_iterations * sizeof(uint64_t));
        uint64_t *normal_timings = malloc(config->num_iterations * sizeof(uint64_t));
        
        if (!cxl_timings || !normal_timings) {
            fprintf(stderr, "[ERROR] Timing buffer allocation failed\n");
            free(cxl_timings);
            free(normal_timings);
            continue;
        }
        
        if (cxl_observe_cxl_latency(cxl_addr, normal_addr, cxl_timings, 
                                    normal_timings, config->num_iterations) >= 0) {
            double latency_diff = 0.0, signal_strength = 0.0;
            
            cxl_analysis_latency_difference(cxl_timings, normal_timings, 
                                           config->num_iterations, 
                                           &latency_diff, &signal_strength);
            
            total_diff += latency_diff;
            total_signal += signal_strength;
            
            fprintf(stdout, "  Latency Difference: %.2f cycles\n", latency_diff);
            fprintf(stdout, "  Signal Strength:    %.2f\n", signal_strength);
        }
        
        free(cxl_timings);
        free(normal_timings);
        if (config->compare_cxl_normal && cxl_addr != normal_addr) {
            cxl_free(cxl_addr, test_size);
        }
        cxl_free(normal_addr, test_size);
    }
    
    /* 汇总统计 */
    fprintf(stdout, "\n[SUMMARY] CXL Latency Test Results:\n");
    fprintf(stdout, "  Average Latency Difference: %.2f cycles\n", 
           total_diff / config->num_rounds);
    fprintf(stdout, "  Average Signal Strength:    %.2f\n", 
           total_signal / config->num_rounds);
    
    return 0;
}

/* ====== 执行完整演示 ====== */
int run_full_demo(test_config_t *config) {
    fprintf(stdout, "\n=============== Full CXL Security Demonstration ===============\n\n");
    
    /* 初始化分析和观测模块 */
    if (cxl_analysis_init(config->output_dir) < 0) {
        fprintf(stderr, "[ERROR] Failed to initialize analysis module\n");
        return -1;
    }
    
    if (cxl_observation_init(OBSERVE_TIMING, config->num_iterations) < 0) {
        fprintf(stderr, "[ERROR] Failed to initialize observation module\n");
        return -1;
    }
    
    /* 依次运行各项测试 */
    fprintf(stdout, "\n[1/4] Running Flush + Reload Test...\n");
    run_flush_reload_test(config);
    
    fprintf(stdout, "\n[2/4] Running Latency Test...\n");
    run_latency_test(config);
    
    fprintf(stdout, "\n[3/4] Running Multi-threading Test...\n");
    if (cxl_framework_test_multithreading(config->num_threads) < 0) {
        fprintf(stderr, "[WARNING] Multi-threading test failed\n");
    }
    
    fprintf(stdout, "\n[4/4] Running Single-thread Test...\n");
    if (cxl_framework_test_singlethreading_isolated() < 0) {
        fprintf(stderr, "[WARNING] Single-thread test failed\n");
    }
    
    /* 清理 */
    cxl_observation_cleanup();
    cxl_analysis_cleanup();
    
    fprintf(stdout, "\n[SUCCESS] Full demonstration completed\n");
    fprintf(stdout, "Results saved to: %s\n", config->output_dir);
    
    return 0;
}

/* ====== 主函数 - 完整的生产级测试框架 ====== */
int main(int argc, char *argv[]) {
    test_config_t config;
    
    /* 解析命令行参数 */
    int parse_result = parse_args(argc, argv, &config);
    if (parse_result != 0) {
        return parse_result > 0 ? 0 : 1;
    }
    
    /* 初始化框架 */
    fprintf(stdout, "\n");
    fprintf(stdout, "╔════════════════════════════════════════════════════════╗\n");
    fprintf(stdout, "║    CXL Memory Side-Channel Detection Framework v1.0     ║\n");
    fprintf(stdout, "║         Linux CXL Security Analysis Platform           ║\n");
    fprintf(stdout, "╚════════════════════════════════════════════════════════╝\n");
    
    if (cxl_framework_init("default") < 0) {
        fprintf(stderr, "[ERROR] Framework initialization failed\n");
        return 1;
    }
    
    /* 打印系统配置 */
    cxl_framework_print_info();
    
    fprintf(stdout, "\n[CONFIG] Test Parameters:\n");
    fprintf(stdout, "  Test Mode:       %d\n", config.test_mode);
    fprintf(stdout, "  Iterations:      %d\n", config.num_iterations);
    fprintf(stdout, "  Rounds:          %d\n", config.num_rounds);
    fprintf(stdout, "  Threads:         %d\n", config.num_threads);
    fprintf(stdout, "  Output Dir:      %s\n", config.output_dir);
    fprintf(stdout, "  Compare CXL/Normal: %s\n", 
           config.compare_cxl_normal ? "Yes" : "No");
    
    /* 执行相应的测试 */
    int result = 0;
    switch (config.test_mode) {
        case 0:
            result = run_flush_reload_test(&config);
            break;
        case 1:
            result = run_latency_test(&config);
            break;
        case 2:
            result = cxl_framework_test_multithreading(config.num_threads);
            break;
        case 3:
            result = cxl_framework_test_singlethreading_isolated();
            break;
        case 4:
            result = run_full_demo(&config);
            break;
        default:
            fprintf(stderr, "[ERROR] Invalid test mode: %d\n", config.test_mode);
            print_usage(argv[0]);
            result = -1;
    }
    
    /* 清理框架 */
    cxl_framework_cleanup();
    
    /* 输出最终状态 */
    fprintf(stdout, "\n╔════════════════════════════════════════════════════════╗\n");
    fprintf(stdout, "║         Test Execution Completed Successfully           ║\n");
    fprintf(stdout, "╚════════════════════════════════════════════════════════╝\n\n");
    
    return result < 0 ? 1 : 0;
}
