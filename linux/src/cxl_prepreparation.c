#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <numa.h>
#include <sched.h>
#include "cxl_prepreparation.h"
#include "cxl_common.h"

/* ====== 初始化配置 ====== */
int cxl_config_init(cxl_config_t *config) {
    if (!config) {
        fprintf(stderr, "[ERROR] Invalid config pointer\n");
        return -1;
    }
    
    memset(config, 0, sizeof(cxl_config_t));
    
    /* 获取系统信息 */
    int num_cpus = cxl_get_num_cpus();
    int num_nodes = cxl_get_num_numa_nodes();
    
    if (num_cpus < 0 || num_nodes < 0) {
        fprintf(stderr, "[ERROR] Failed to get system information\n");
        return -1;
    }
    
    /* 设置默认配置 */
    config->numa_node_normal = 0;
    config->numa_node_cxl = (num_nodes > 1) ? 1 : 0;
    
    config->thread_placement = CROSS_CORE;
    config->data_placement = PLACEMENT_NORMAL_NODE;
    
    /* CPU 绑定：使用合理的默认值 */
    config->attacker_cpu = 0;
    config->victim_cpu = num_cpus / 2;  /* 使用中间的 CPU */
    config->probe_cpu = (num_cpus / 2) + 1;
    config->monitor_cpu = num_cpus - 1;
    
    config->prefetcher_enabled = 1;  /* 默认启用预取器 */
    config->isolcpus_enabled = 0;    /* 默认不启用隔离 */
    
    /* 工作参数 */
    config->iterations = 1000;
    config->warmup_iterations = 100;
    config->sample_size = 1000;
    
    fprintf(stdout, "[INFO] Configuration initialized\n");
    
    return 0;
}

int cxl_set_numa_nodes(cxl_config_t *config, int normal_node, int cxl_node) {
    if (!config) {
        fprintf(stderr, "[ERROR] Invalid config pointer\n");
        return -1;
    }
    
    int num_nodes = cxl_get_num_numa_nodes();
    if (normal_node >= num_nodes || cxl_node >= num_nodes) {
        fprintf(stderr, "[ERROR] Invalid NUMA node IDs\n");
        return -1;
    }
    
    config->numa_node_normal = normal_node;
    config->numa_node_cxl = cxl_node;
    
    fprintf(stdout, "[INFO] NUMA nodes set: normal=%d, cxl=%d\n", normal_node, cxl_node);
    
    return 0;
}

int cxl_set_thread_placement(cxl_config_t *config, thread_placement_t placement) {
    if (!config) {
        fprintf(stderr, "[ERROR] Invalid config pointer\n");
        return -1;
    }
    
    config->thread_placement = placement;
    
    const char *placement_names[] = {"CROSS_CORE", "DIFFERENT_THREAD", "SAME_THREAD"};
    fprintf(stdout, "[INFO] Thread placement set to: %s\n", placement_names[placement]);
    
    return 0;
}

int cxl_set_data_placement(cxl_config_t *config, data_placement_t placement) {
    if (!config) {
        fprintf(stderr, "[ERROR] Invalid config pointer\n");
        return -1;
    }
    
    config->data_placement = placement;
    
    const char *placement_names[] = {"NORMAL_NODE", "CXL_MEMORY", "LOCAL_CACHE"};
    fprintf(stdout, "[INFO] Data placement set to: %s\n", placement_names[placement]);
    
    return 0;
}

/* ====== 预取器配置 ====== */
int cxl_configure_prefetcher(int enabled) {
    /* 在 Linux 上，可以通过写入 MSR 或 sysfs 来配置预取器 */
    /* 这需要 root 权限并使用 msr-tools 或直接 /dev/msr */
    
    if (enabled) {
        fprintf(stdout, "[INFO] Prefetcher configuration: ENABLED\n");
        /* 实现：写入 MSR 0x1A0 的相关位 */
    } else {
        fprintf(stdout, "[INFO] Prefetcher configuration: DISABLED\n");
        /* 实现：清除 MSR 0x1A0 的相关位 */
    }
    
    return 0;
}

/* ====== isolcpus 配置 ====== */
int cxl_configure_isolcpus(int enabled) {
    /* isolcpus 需要在内核参数中配置，通过 /proc/cmdline 检查 */
    
    if (access("/proc/cmdline", F_OK) == 0) {
        FILE *file = fopen("/proc/cmdline", "r");
        if (file) {
            char cmdline[1024];
            if (fgets(cmdline, sizeof(cmdline), file) != NULL) {
                if (strstr(cmdline, "isolcpus=") != NULL) {
                    fprintf(stdout, "[INFO] isolcpus is configured in kernel parameters\n");
                } else if (enabled) {
                    fprintf(stdout, "[WARNING] isolcpus NOT found in kernel parameters, "
                                   "reboot with isolcpus=... to enable\n");
                }
            }
            fclose(file);
        }
    }
    
    return 0;
}

int cxl_set_cpu_binding(cxl_config_t *config, int attacker_cpu, int victim_cpu) {
    if (!config) {
        fprintf(stderr, "[ERROR] Invalid config pointer\n");
        return -1;
    }
    
    int num_cpus = cxl_get_num_cpus();
    if (attacker_cpu >= num_cpus || victim_cpu >= num_cpus) {
        fprintf(stderr, "[ERROR] CPU IDs out of range (max: %d)\n", num_cpus - 1);
        return -1;
    }
    
    config->attacker_cpu = attacker_cpu;
    config->victim_cpu = victim_cpu;
    
    fprintf(stdout, "[INFO] CPU binding set: attacker=%d, victim=%d\n", attacker_cpu, victim_cpu);
    
    return 0;
}

int cxl_get_num_cpus(void) {
    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    return (num_cpus > 0) ? num_cpus : -1;
}

int cxl_get_num_numa_nodes(void) {
    int num_nodes = numa_num_configured_nodes();
    return (num_nodes > 0) ? num_nodes : -1;
}

/* ====== 配置打印 ====== */
void cxl_print_config(const cxl_config_t *config) {
    if (!config) {
        fprintf(stderr, "[ERROR] Invalid config pointer\n");
        return;
    }
    
    const char *thread_placement_names[] = {"CROSS_CORE", "DIFFERENT_THREAD", "SAME_THREAD"};
    const char *data_placement_names[] = {"NORMAL_NODE", "CXL_MEMORY", "LOCAL_CACHE"};
    
    fprintf(stdout, "\n========== CXL Framework Configuration ==========\n");
    fprintf(stdout, "NUMA Configuration:\n");
    fprintf(stdout, "  Normal Node ID:     %d\n", config->numa_node_normal);
    fprintf(stdout, "  CXL Node ID:        %d\n", config->numa_node_cxl);
    fprintf(stdout, "\nThread Configuration:\n");
    fprintf(stdout, "  Thread Placement:   %s\n", thread_placement_names[config->thread_placement]);
    fprintf(stdout, "  Attacker CPU:       %d\n", config->attacker_cpu);
    fprintf(stdout, "  Victim CPU:         %d\n", config->victim_cpu);
    fprintf(stdout, "  Probe CPU:          %d\n", config->probe_cpu);
    fprintf(stdout, "  Monitor CPU:        %d\n", config->monitor_cpu);
    fprintf(stdout, "\nData Configuration:\n");
    fprintf(stdout, "  Data Placement:     %s\n", data_placement_names[config->data_placement]);
    fprintf(stdout, "\nSystem Configuration:\n");
    fprintf(stdout, "  Prefetcher:         %s\n", config->prefetcher_enabled ? "ENABLED" : "DISABLED");
    fprintf(stdout, "  isolcpus:           %s\n", config->isolcpus_enabled ? "ENABLED" : "DISABLED");
    fprintf(stdout, "\nExperiment Parameters:\n");
    fprintf(stdout, "  Iterations:         %lu\n", config->iterations);
    fprintf(stdout, "  Warmup Iterations:  %lu\n", config->warmup_iterations);
    fprintf(stdout, "  Sample Size:        %u\n", config->sample_size);
    fprintf(stdout, "=================================================\n\n");
}

/* ====== 配置验证 ====== */
int cxl_validate_config(const cxl_config_t *config) {
    if (!config) {
        fprintf(stderr, "[ERROR] Invalid config pointer\n");
        return -1;
    }
    
    int num_cpus = cxl_get_num_cpus();
    int num_nodes = cxl_get_num_numa_nodes();
    
    if (config->attacker_cpu >= num_cpus) {
        fprintf(stderr, "[ERROR] Attacker CPU %d >= number of CPUs %d\n", 
                config->attacker_cpu, num_cpus);
        return -1;
    }
    
    if (config->victim_cpu >= num_cpus) {
        fprintf(stderr, "[ERROR] Victim CPU %d >= number of CPUs %d\n", 
                config->victim_cpu, num_cpus);
        return -1;
    }
    
    if (config->numa_node_normal >= num_nodes) {
        fprintf(stderr, "[ERROR] Normal node %d >= number of NUMA nodes %d\n", 
                config->numa_node_normal, num_nodes);
        return -1;
    }
    
    if (config->numa_node_cxl >= num_nodes) {
        fprintf(stderr, "[ERROR] CXL node %d >= number of NUMA nodes %d\n", 
                config->numa_node_cxl, num_nodes);
        return -1;
    }
    
    fprintf(stdout, "[INFO] Configuration validation passed\n");
    return 0;
}

int cxl_check_system_support(void) {
    int num_nodes = cxl_get_num_numa_nodes();
    if (num_nodes < 2) {
        fprintf(stderr, "[WARNING] System has less than 2 NUMA nodes, CXL support may be limited\n");
        return 0;
    }
    fprintf(stdout, "[INFO] CXL system support check passed\n");
    return 1;
}

int cxl_get_cxl_node(void) {
    int num_nodes = cxl_get_num_numa_nodes();
    if (num_nodes > 1) {
        return 1;  /* 假设 CXL Memory 在节点 1 */
    }
    return -1;
}

int cxl_setup_multithreading(cxl_config_t *config, int num_threads) {
    if (!config || num_threads <= 0) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    int num_cpus = cxl_get_num_cpus();
    if (num_threads > num_cpus) {
        fprintf(stderr, "[WARNING] num_threads %d > available CPUs %d\n", num_threads, num_cpus);
    }
    
    /* 为多线程配置设置较低的隔离要求 */
    config->prefetcher_enabled = 1;
    config->isolcpus_enabled = 0;
    
    fprintf(stdout, "[INFO] Multithreading setup for %d threads\n", num_threads);
    
    return 0;
}

int cxl_setup_singlethreading(cxl_config_t *config) {
    if (!config) {
        fprintf(stderr, "[ERROR] Invalid config pointer\n");
        return -1;
    }
    
    /* 单线程测试应该尽可能隔离 */
    config->prefetcher_enabled = 0;
    config->isolcpus_enabled = 1;
    
    fprintf(stdout, "[INFO] Single-threading setup\n");
    
    return 0;
}
