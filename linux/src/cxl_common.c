#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <pthread.h>
#include <numa.h>
#include <sched.h>
#include <errno.h>
#include "cxl_common.h"

/* ====== NUMA 内存分配 ====== */
void *cxl_malloc_on_node(size_t size, int node) {
    if (node < 0) {
        return malloc(size);
    }
    
    void *ptr;
    struct bitmask *nodeset;
    
    nodeset = numa_allocate_nodemask();
    if (!nodeset) {
        fprintf(stderr, "[ERROR] Failed to allocate node mask\n");
        return NULL;
    }
    
    numa_bitmask_setbit(nodeset, node);
    ptr = numa_alloc_onnode(size, node);
    numa_free_nodemask(nodeset);
    
    if (!ptr) {
        fprintf(stderr, "[ERROR] Failed to allocate %zu bytes on node %d\n", size, node);
        return NULL;
    }
    
    return ptr;
}

void cxl_free(void *ptr, size_t size) {
    if (!ptr) return;
    
    numa_free(ptr, size);
}

/* ====== CPU 亲和性绑定 ====== */
int cxl_bind_to_cpu(int cpu_id) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu_id, &set);
    
    if (sched_setaffinity(0, sizeof(cpu_set_t), &set) < 0) {
        fprintf(stderr, "[ERROR] Failed to bind to CPU %d: %s\n", cpu_id, strerror(errno));
        return -1;
    }
    
    return 0;
}

int cxl_bind_to_node(int node_id) {
    if (node_id < 0) return 0;
    
    struct bitmask *nodeset = numa_allocate_nodemask();
    if (!nodeset) {
        fprintf(stderr, "[ERROR] Failed to allocate node mask\n");
        return -1;
    }
    
    numa_bitmask_setbit(nodeset, node_id);
    if (numa_sched_setaffinity(0, nodeset) < 0) {
        fprintf(stderr, "[ERROR] Failed to bind to node %d: %s\n", node_id, strerror(errno));
        numa_free_nodemask(nodeset);
        return -1;
    }
    
    numa_free_nodemask(nodeset);
    return 0;
}

/* ====== 系统信息查询 ====== */
int cxl_get_num_cpus(void) {
    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_cpus < 0) {
        fprintf(stderr, "[ERROR] Failed to get number of CPUs\n");
        return -1;
    }
    return num_cpus;
}

int cxl_get_num_numa_nodes(void) {
    int num_nodes = numa_num_configured_nodes();
    if (num_nodes < 0) {
        fprintf(stderr, "[ERROR] Failed to get number of NUMA nodes\n");
        return -1;
    }
    return num_nodes;
}

/* ====== 获取 APIC ID ====== */
static uint32_t cxl_get_apic_id(void) {
    uint32_t eax, edx;
    
    asm volatile(
        "cpuid"
        : "=a" (eax), "=d" (edx)
        : "a" (0x1)
        : "ebx", "ecx"
    );
    
    return (edx >> 24) & 0xFF;
}

/* ====== 检查 CXL 支持 ====== */
int cxl_check_system_support(void) {
    /* 检查系统是否有多个 NUMA 节点 */
    int num_nodes = cxl_get_num_numa_nodes();
    if (num_nodes < 2) {
        fprintf(stderr, "[WARNING] System has less than 2 NUMA nodes\n");
        return 0;
    }
    
    /* 进一步的 CXL 检验可以通过 sysfs 进行 */
    /* /sys/bus/cxl/devices/ 查看 CXL 设备 */
    
    return 1;
}

int cxl_get_cxl_node(void) {
    /* 简单实现：假设 CXL Memory 在节点 1 */
    /* 实际应该通过 sysfs 或 CXL 驱动获取 */
    if (cxl_get_num_numa_nodes() > 1) {
        return 1;
    }
    return -1;
}

/* ====== 时间相关函数 ====== */
uint64_t cxl_rdtscp(uint32_t *cpu_id) {
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

void cxl_mfence(void) {
    asm volatile("mfence" : : : "memory");
}

void cxl_lfence(void) {
    asm volatile("lfence" : : : "memory");
}

void cxl_serialization_point(void) {
    cxl_mfence();
}

uint64_t cxl_access_time(uint64_t start, uint64_t end) {
    return (end > start) ? (end - start) : 0;
}

/* ====== 日志与调试 ====== */
void cxl_log_info(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stdout, "[INFO] ");
    vfprintf(stdout, format, args);
    fprintf(stdout, "\n");
    va_end(args);
}

void cxl_log_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "[ERROR] ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

void cxl_log_warning(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "[WARNING] ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}
