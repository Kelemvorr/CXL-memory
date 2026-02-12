#ifndef CXL_OBSERVATION_H
#define CXL_OBSERVATION_H

#include "cxl_common.h"

/* ====== 侧信道观测接口 ====== */

/**
 * @brief 初始化观测模块
 * @param observation_type 观测类型（时间/模式/痕迹）
 * @param buffer_size 缓冲区大小
 * @return 0 成功，-1 失败
 */
int cxl_observation_init(observation_type_t observation_type, size_t buffer_size);

/**
 * @brief 观测内存访问时间
 * @param addr 要观测的内存地址
 * @param num_samples 样本数量
 * @param samples 返回的时间样本数组
 * @return 实际采集的样本数
 */
int cxl_observe_access_timing(void *addr, int num_samples, uint64_t *samples);

/**
 * @brief 观测缓存命中/未命中模式
 * @param addrs 观测地址数组
 * @param num_addrs 地址数量
 * @param num_probes 每个地址的探测次数
 * @param hit_patterns 返回的模式数据（位数组表示命中/未命中）
 * @return 0 成功，-1 失败
 */
int cxl_observe_cache_pattern(void **addrs, int num_addrs, int num_probes, 
                              uint8_t *hit_patterns);

/**
 * @brief 观测内存访问痕迹（哪些地址被访问）
 * @param addr_range_start 观测范围起始地址
 * @param addr_range_size 范围大小
 * @param num_traces 返回的访问痕迹数据
 * @param traces 访问痕迹数据（相对偏移量）
 * @return 实际捕获的访问痕迹数
 */
int cxl_observe_access_trace(void *addr_range_start, size_t addr_range_size,
                             int *num_traces, uint32_t *traces);

/**
 * @brief 高精度时间戳采集
 * @param samples 返回的采样数组
 * @param num_samples 采样数量
 * @return 实际采集的样本数
 */
int cxl_observe_rdtscp_samples(timing_sample_t *samples, int num_samples);

/**
 * @brief 观测特定地址的重复访问间隔
 * @param addr 要观测的地址
 * @param max_intervals 最多记录的间隔数
 * @param intervals 返回的间隔数组（cycles）
 * @return 记录的间隔数
 */
int cxl_observe_access_intervals(void *addr, int max_intervals, uint64_t *intervals);

/**
 * @brief 观测攻击者与受害者的时间交集
 * @param attacker_timing 攻击者的时间戳
 * @param victim_timing 受害者的时间戳
 * @param overlap_time 返回重叠时序窗口
 * @return 0 有重叠，1 无重叠，-1 错误
 */
int cxl_observe_timing_overlap(uint64_t attacker_timing, uint64_t victim_timing,
                               uint64_t *overlap_time);

/**
 * @brief 观测缓存行冲突
 * @param addrs 地址数组
 * @param num_addrs 地址数量
 * @param conflicts 返回冲突信息
 * @return 0 成功，-1 失败
 */
int cxl_observe_cache_conflicts(void **addrs, int num_addrs, uint8_t *conflicts);

/**
 * @brief 观测 L3 缓存的时间特性
 * @param addr 目标地址
 * @param num_measurements 测量次数
 * @param timings 返回的时间测量数据
 * @return 0 成功，-1 失败
 */
int cxl_observe_l3_timing(void *addr, int num_measurements, uint64_t *timings);

/**
 * @brief 观测 CXL Memory 的访问延迟
 * @param cxl_addr CXL 内存地址
 * @param normal_addr 普通内存地址（用于对比）
 * @param cxl_timings CXL 访问时间数组
 * @param normal_timings 普通内存访问时间数组
 * @param num_samples 样本数量
 * @return 0 成功，-1 失败
 */
int cxl_observe_cxl_latency(void *cxl_addr, void *normal_addr,
                            uint64_t *cxl_timings, uint64_t *normal_timings,
                            int num_samples);

/**
 * @brief 计算时间序列的统计特征
 * @param samples 时间样本数组
 * @param num_samples 样本数量
 * @param min 返回最小值
 * @param max 返回最大值
 * @param mean 返回平均值
 * @param stddev 返回标准差
 * @return 0 成功，-1 失败
 */
int cxl_observe_statistics(const uint64_t *samples, int num_samples,
                           uint64_t *min, uint64_t *max, 
                           double *mean, double *stddev);

/**
 * @brief 检测时间分布中的异常
 * @param samples 时间样本数组
 * @param num_samples 样本数量
 * @param threshold 异常阈值（标准差倍数）
 * @param anomalies 返回异常位置的位掩码
 * @return 异常样本数
 */
int cxl_observe_detect_anomalies(const uint64_t *samples, int num_samples,
                                 double threshold, uint8_t *anomalies);

/**
 * @brief 启动实时观测（后台线程）
 * @param target_addr 观测目标地址
 * @param observation_type 观测类型
 * @param callback 观测数据回调函数
 * @return 0 成功，-1 失败
 */
typedef void (*observation_callback_t)(const observation_data_t *data, void *ctx);

int cxl_observe_realtime_start(void *target_addr, observation_type_t observation_type,
                               observation_callback_t callback, void *ctx);

/**
 * @brief 停止实时观测
 * @return 0 成功，-1 失败
 */
int cxl_observe_realtime_stop(void);

/**
 * @brief 清空观测缓冲区
 * @return 0 成功，-1 失败
 */
int cxl_observation_clear_buffer(void);

/**
 * @brief 获取观测缓冲区中的数据
 * @param buffer 返回的数据缓冲
 * @param buffer_size 缓冲大小
 * @return 返回的数据大小
 */
int cxl_observation_get_data(void *buffer, size_t buffer_size);

/**
 * @brief 清理观测模块
 * @return 0 成功，-1 失败
 */
int cxl_observation_cleanup(void);

#endif /* CXL_OBSERVATION_H */
