#ifndef CXL_PREPREPARATION_H
#define CXL_PREPREPARATION_H

#include "cxl_common.h"

/* ====== 初始化与配置 ====== */

/**
 * @brief 初始化 CXL 框架配置
 * @param config 框架配置结构指针
 * @return 0 成功，-1 失败
 */
int cxl_config_init(cxl_config_t *config);

/**
 * @brief 设置 NUMA 节点配置
 * @param config 框架配置
 * @param normal_node 普通 NUMA 节点 ID
 * @param cxl_node CXL Memory 节点 ID
 * @return 0 成功，-1 失败
 */
int cxl_set_numa_nodes(cxl_config_t *config, int normal_node, int cxl_node);

/**
 * @brief 设置攻击者和受害者的线程位置
 * @param config 框架配置
 * @param placement 线程位置类型（跨核心/不同线程/同线程）
 * @return 0 成功，-1 失败
 */
int cxl_set_thread_placement(cxl_config_t *config, thread_placement_t placement);

/**
 * @brief 设置数据放置位置
 * @param config 框架配置
 * @param placement 数据放置类型（普通节点/CXL内存/L3缓存）
 * @return 0 成功，-1 失败
 */
int cxl_set_data_placement(cxl_config_t *config, data_placement_t placement);

/**
 * @brief 配置预取器（Prefetcher）
 * @param enabled 1 启用，0 禁用
 * @return 0 成功，-1 失败
 */
int cxl_configure_prefetcher(int enabled);

/**
 * @brief 配置 isolcpus 隔离核心
 * @param cpu_mask 隔离的核心掩码
 * @param enabled 1 启用，0 禁用
 * @return 0 成功，-1 失败
 */
int cxl_configure_isolcpus(int enabled);

/**
 * @brief 设置工作线程的 CPU 绑定
 * @param config 框架配置
 * @param attacker_cpu 攻击者线程 CPU
 * @param victim_cpu 受害者线程 CPU
 * @return 0 成功，-1 失败
 */
int cxl_set_cpu_binding(cxl_config_t *config, int attacker_cpu, int victim_cpu);

/**
 * @brief 获取可用的 CPU 核心数
 * @return 核心数，失败返回 -1
 */
int cxl_get_num_cpus(void);

/**
 * @brief 获取 NUMA 节点数
 * @return 节点数，失败返回 -1
 */
int cxl_get_num_numa_nodes(void);

/**
 * @brief 打印当前配置信息
 * @param config 框架配置
 */
void cxl_print_config(const cxl_config_t *config);

/**
 * @brief 验证配置合法性
 * @param config 框架配置
 * @return 0 合法，-1 非法
 */
int cxl_validate_config(const cxl_config_t *config);

/**
 * @brief 检查系统支持 CXL
 * @return 1 支持，0 不支持，-1 错误
 */
int cxl_check_system_support(void);

/**
 * @brief 获取 CXL Memory 节点信息
 * @return 节点 ID，失败返回 -1
 */
int cxl_get_cxl_node(void);

/**
 * @brief 配置多线程测试环境
 * @param config 框架配置
 * @param num_threads 线程数量
 * @return 0 成功，-1 失败
 */
int cxl_setup_multithreading(cxl_config_t *config, int num_threads);

/**
 * @brief 配置单线程测试环境
 * @param config 框架配置
 * @return 0 成功，-1 失败
 */
int cxl_setup_singlethreading(cxl_config_t *config);

#endif /* CXL_PREPREPARATION_H */
