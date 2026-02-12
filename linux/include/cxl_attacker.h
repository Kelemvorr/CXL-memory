#ifndef CXL_ATTACKER_H
#define CXL_ATTACKER_H

#include "cxl_common.h"

/* ====== 类型定义 ====== */
/**
 * @brief 攻击函数指针类型
 */
typedef int (*attack_func_t)(void *arg, attack_result_t *result);

/* ====== 攻击者角色操作接口 ====== */

/**
 * @brief 初始化攻击者线程环境
 * @param cpu_id 攻击者绑定的 CPU 核心 ID
 * @return 0 成功，-1 失败
 */
int cxl_attacker_init(int cpu_id);

/**
 * @brief 攻击者执行 Flush + Reload 攻击
 * @param victim_data 受害者数据地址
 * @param result 返回攻击结果
 * @return 0 成功，-1 失败
 */
int cxl_attacker_flush_reload(void *victim_data, attack_result_t *result);

/**
 * @brief 攻击者执行 Evict + Time 攻击
 * @param victim_data 受害者数据地址  
 * @param evict_set 驱逐集合地址数组
 * @param evict_set_size 驱逐集合大小
 * @param result 返回攻击结果
 * @return 0 成功，-1 失败
 */
int cxl_attacker_evict_time(void *victim_data, void **evict_set, 
                            int evict_set_size, attack_result_t *result);

/**
 * @brief 攻击者执行 Spectre 攻击（推测执行泄露）
 * @param gadget_addr Spectre Gadget 地址
 * @param condition 推测条件
 * @param result 返回攻击结果
 * @return 0 成功，-1 失败
 */
int cxl_attacker_spectre(void *gadget_addr, int condition, attack_result_t *result);

/**
 * @brief 攻击者执行 Meltdown 攻击（越权内存读取）
 * @param target_addr 目标内存地址（受保护）
 * @param result 返回攻击结果
 * @return 0 成功，-1 失败
 */
int cxl_attacker_meltdown(void *target_addr, attack_result_t *result);

/**
 * @brief 攻击者执行 Prime + Probe 攻击
 * @param target_set 目标集合地址数组
 * @param set_size 目标集合大小
 * @param result 返回攻击结果
 * @return 0 成功，-1 失败
 */
int cxl_attacker_prime_probe(void **target_set, int set_size, attack_result_t *result);

/**
 * @brief 攻击者探测特定内存地址的缓存状态
 * @param addrs 要探测的地址数组
 * @param num_addrs 地址数量
 * @param timings 返回的测时数据数组
 * @param num_timings 返回数据数量
 * @return 0 成功，-1 失败
 */
int cxl_attacker_probe_addresses(void **addrs, int num_addrs, 
                                 uint64_t *timings, int *num_timings);

/**
 * @brief 攻击者执行驱逐操作
 * @param evict_set 驱逐集合地址数组
 * @param set_size 集合大小
 * @return 0 成功，-1 失败
 */
int cxl_attacker_evict_set(void **evict_set, int set_size);

/**
 * @brief 攻击者执行预热操作
 * @param warmup_iterations 预热迭代次数
 * @return 0 成功，-1 失败
 */
int cxl_attacker_warmup(uint64_t warmup_iterations);

/**
 * @brief 攻击者执行时序旁路
 * @param victim_addr 受害者内存地址
 * @param threshold 时间阈值（用于判断命中/未命中）
 * @param num_samples 样本数量
 * @param samples 返回的时间样本数组
 * @return 0 成功，-1 失败
 */
int cxl_attacker_timing_sidechannel(void *victim_addr, uint64_t threshold,
                                    int num_samples, uint64_t *samples);

/**
 * @brief 攻击者执行噪声消除操作
 * @param raw_samples 原始样本数组
 * @param num_samples 样本数量
 * @param filtered_samples 返回的滤波样本数组
 * @return 0 成功，-1 失败
 */
int cxl_attacker_noise_filtering(const uint64_t *raw_samples, int num_samples,
                                 uint64_t *filtered_samples);

/**
 * @brief 攻击者收集访问模式
 * @param victim_addr 受害者地址
 * @param num_probes 探测次数
 * @param pattern 返回的访问模式
 * @return 0 成功，-1 失败
 */
int cxl_attacker_collect_pattern(void *victim_addr, int num_probes, uint8_t *pattern);

/**
 * @brief 攻击者清理工作状态
 * @return 0 成功，-1 失败
 */
int cxl_attacker_cleanup(void);

/**
 * @brief 攻击者线程主循环
 * @param arg 线程参数
 * @return NULL
 */
void *cxl_attacker_thread_main(void *arg);

/**
 * @brief 攻击者重复执行某种攻击
 * @param attack_func 攻击函数指针
 * @param num_repeats 重复次数
 * @param results 返回的结果数组
 * @return 0 成功，-1 失败
 */
int cxl_attacker_repeat_attack(attack_func_t attack_func, void *arg,
                               int num_repeats, attack_result_t *results);

/**
 * @brief 攻击者统计攻击成功率
 * @param results 攻击结果数组
 * @param num_results 结果数量
 * @return 成功率（0-100%）
 */
float cxl_attacker_success_rate(const attack_result_t *results, int num_results);

#endif /* CXL_ATTACKER_H */
