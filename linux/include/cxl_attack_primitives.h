#ifndef CXL_ATTACK_PRIMITIVES_H
#define CXL_ATTACK_PRIMITIVES_H

#include "cxl_common.h"

/* ====== 侧信道攻击原语基础操作 ====== */

/**
 * @brief Clflush 缓存清除原语
 * @param addr 要清除的内存地址
 */
void cxl_flush_clflush(void *addr);

/**
 * @brief Clflushopt 优化缓存清除原语
 * @param addr 要清除的内存地址
 */
void cxl_flush_clflushopt(void *addr);

/**
 * @brief 批量 Clflush 操作
 * @param start_addr 起始地址
 * @param end_addr 结束地址
 */
void cxl_flush_range(void *start_addr, void *end_addr);

/**
 * @brief Probe 操作：通过访问时间测量缓存命中/缺失
 * @param addr 要 probe 的内存地址
 * @param out_time 返回访问时间（cycles）
 * @return 访问时间
 */
uint64_t cxl_probe_access_time(void *addr, uint64_t *out_time);

/**
 * @brief 批量 Probe 操作
 * @param addrs 地址数组
 * @param num_addrs 地址数量
 * @param timings 返回的时间数组
 */
void cxl_probe_multiple(void **addrs, int num_addrs, uint64_t *timings);

/**
 * @brief Reload 操作：将数据重新加载到缓存
 * @param addr 要 reload 的地址
 */
void cxl_reload(void *addr);

/**
 * @brief 侧信道攻击：Load 操作
 * @param addr 待访问的内存地址
 * @return 装载时间
 */
uint64_t cxl_attack_load(void *addr);

/**
 * @brief 侧信道攻击：Store 操作
 * @param addr 待写入的内存地址
 * @param value 写入值
 */
void cxl_attack_store(void *addr, uint64_t value);

/**
 * @brief 侧信道攻击：Prefetch 操作
 * @param addr 预取地址
 */
void cxl_attack_prefetch(void *addr);

/**
 * @brief 侧信道攻击：Mfence 序列化
 */
void cxl_attack_mfence(void);

/**
 * @brief 侧信道攻击：Lfence 序列化
 */
void cxl_attack_lfence(void);

/**
 * @brief 执行受害者的内存访问操作
 * @param addr 内存地址
 * @param is_write 1 表示写操作，0 表示读操作
 * @return 访问时间
 */
uint64_t cxl_victim_memory_access(void *addr, int is_write);

/**
 * @brief 执行随机内存访问以破坏取证
 * @param start_addr 内存范围起始地址
 * @param size 内存范围大小
 * @param num_accesses 访问次数
 */
void cxl_random_memory_access(void *start_addr, size_t size, int num_accesses);

/**
 * @brief 驱逐级别特定的缓存
 * @param level 缓存级别（1 L1, 2 L2, 3 L3）
 * @param evict_addr 用于驱逐的地址
 */
void cxl_evict_cache_level(int level, void *evict_addr);

/**
 * @brief 执行 Flush + Reload 组合
 * @param addr 目标地址
 * @return Reload 访问时间
 */
uint64_t cxl_flush_reload(void *addr);

/**
 * @brief 执行 Flush + Flush 组合
 * @param addr 目标地址
 */
void cxl_flush_double(void *addr);

/**
 * @brief 执行 Evict + Time 组合
 * @param evict_addr 驱逐地址
 * @param probe_addr Probe 地址
 * @return Probe 访问时间
 */
uint64_t cxl_evict_time(void *evict_addr, void *probe_addr);

/**
 * @brief 执行 Spectre 风格的侧信道组合
 * @param cond_addr 条件检查地址
 * @param true_addr 为真时访问的地址
 * @param probe_addr 要 probe 的地址
 * @return Probe 访问时间
 */
uint64_t cxl_spectre_variant(void *cond_addr, void *true_addr, void *probe_addr);

/**
 * @brief 设置测时阈值（用于区分命中与未命中）
 * @param threshold 以周期为单位的阈值
 */
void cxl_set_timing_threshold(uint64_t threshold);

/**
 * @brief 获取测时阈值
 * @return 当前阈值（周期）
 */
uint64_t cxl_get_timing_threshold(void);

/**
 * @brief 执行原子操作（用于同步）
 * @param addr 目标地址
 */
void cxl_atomic_operation(void *addr);

#endif /* CXL_ATTACK_PRIMITIVES_H */
