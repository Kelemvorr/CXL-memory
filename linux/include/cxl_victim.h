#ifndef CXL_VICTIM_H
#define CXL_VICTIM_H

#include "cxl_common.h"

/* ====== 类型定义 ====== */
/**
 * @brief 受害者访问模式统计结构
 */
typedef struct {
    uint64_t total_accesses;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t avg_latency;
    uint64_t max_latency;
    uint64_t min_latency;
} victim_stats_t;

/* ====== 受害者角色操作接口 ====== */

/**
 * @brief 初始化受害者线程环境
 * @param cpu_id 受害者绑定的 CPU 核心 ID
 * @return 0 成功，-1 失败
 */
int cxl_victim_init(int cpu_id);

/**
 * @brief 受害者执行内存访问序列
 * @param addrs 内存地址数组
 * @param num_addrs 地址数量
 * @param access_pattern 访问模式（顺序/随机/步长）
 * @return 0 成功，-1 失败
 */
int cxl_victim_memory_sequence(void **addrs, int num_addrs, const char *access_pattern);

/**
 * @brief 受害者执行单次内存访问
 * @param addr 内存地址
 * @param is_write 1 写操作，0 读操作
 * @return 访问时间
 */
uint64_t cxl_victim_single_access(void *addr, int is_write);

/**
 * @brief 受害者执行通用计算任务
 * @param duration_us 运行时间（微秒）
 * @param workload_type 工作负载类型（CPU 密集/内存密集）
 * @return 0 成功，-1 失败
 */
int cxl_victim_workload(uint64_t duration_us, const char *workload_type);

/**
 * @brief 受害者执行数据加密操作（通常包含敏感内存访问）
 * @param key 密钥数据
 * @param plaintext 明文数据
 * @param ciphertext 密文输出缓冲
 * @return 0 成功，-1 失败
 */
int cxl_victim_encrypt_operation(const void *key, const void *plaintext, void *ciphertext);

/**
 * @brief 受害者执行数据查表操作
 * @param table 查表数据
 * @param table_size 表大小
 * @param indices 查询索引数组
 * @param num_indices 索引数量
 * @return 0 成功，-1 失败
 */
int cxl_victim_lookup_operation(void *table, size_t table_size, 
                                const uint32_t *indices, int num_indices);

/**
 * @brief 受害者执行分支操作（导致 Spectre 攻击风险）
 * @param condition 分支条件
 * @param true_data 条件为真时访问的数据
 * @param false_data 条件为假时访问的数据
 * @return 0 成功，-1 失败
 */
int cxl_victim_branch_operation(int condition, void *true_data, void *false_data);

/**
 * @brief 受害者执行串联访问（访问有数据依赖的地址）
 * @param chain_addrs 链式地址数组
 * @param chain_size 链长度
 * @return 0 成功，-1 失败
 */
int cxl_victim_chain_access(void **chain_addrs, int chain_size);

/**
 * @brief 受害者清理工作状态
 * @return 0 成功，-1 失败
 */
int cxl_victim_cleanup(void);

/**
 * @brief 受害者进入空闲状态
 * @param idle_duration_us 空闲时间（微秒）
 */
void cxl_victim_idle(uint64_t idle_duration_us);

/**
 * @brief 受害者获取访问模式统计
 * @param stats 统计信息结构
 * @return 0 成功，-1 失败
 */
int cxl_victim_get_statistics(victim_stats_t *stats);

/**
 * @brief 受害者线程主循环
 * @param arg 线程参数
 * @return NULL
 */
void *cxl_victim_thread_main(void *arg);

/**
 * @brief 受害者接收时序操作指令（用于同步）
 * @return 下一个操作的指令代码
 */
int cxl_victim_wait_for_command(void);

/**
 * @brief 受害者执行定时循环攻击
 * @param addr 目标地址
 * @param iterations 迭代次数
 * @param interval_us 操作间隔（微秒）
 * @return 0 成功，-1 失败
 */
int cxl_victim_timed_loop(void *addr, uint64_t iterations, uint64_t interval_us);

#endif /* CXL_VICTIM_H */
