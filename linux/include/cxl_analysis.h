#ifndef CXL_ANALYSIS_H
#define CXL_ANALYSIS_H

#include "cxl_common.h"

/* ====== 数据分析与可视化接口 ====== */

/**
 * @brief 初始化分析模块
 * @param output_dir 输出目录路径
 * @return 0 成功，-1 失败
 */
int cxl_analysis_init(const char *output_dir);

/**
 * @brief 计算时间序列的基本统计
 * @param timings 时间数据数组
 * @param num_samples 样本数量
 * @param min 返回最小值
 * @param max 返回最大值
 * @param mean 返回平均值
 * @param median 返回中位数
 * @param stddev 返回标准差
 * @return 0 成功，-1 失败
 */
int cxl_analysis_compute_statistics(const uint64_t *timings, int num_samples,
                                    uint64_t *min, uint64_t *max, 
                                    double *mean, double *median, double *stddev);

/**
 * @brief 计算两组时间序列的对比
 * @param timings_a 第一组时间数据
 * @param num_a 第一组样本数
 * @param timings_b 第二组时间数据
 * @param num_b 第二组样本数
 * @param difference 返回平均差异
 * @param p_value 返回统计 p 值（用于判断差异显著性）
 * @return 0 成功，-1 失败
 */
int cxl_analysis_compare_distributions(const uint64_t *timings_a, int num_a,
                                       const uint64_t *timings_b, int num_b,
                                       double *difference, double *p_value);

/**
 * @brief 将时间分布数据导出为 CSV 格式
 * @param timings 时间数据数组
 * @param num_samples 样本数量
 * @param label 数据标签
 * @param output_file 输出文件路径
 * @return 0 成功，-1 失败
 */
int cxl_analysis_export_csv(const uint64_t *timings, int num_samples,
                            const char *label, const char *output_file);

/**
 * @brief 生成时间分布直方图
 * @param timings 时间数据数组
 * @param num_samples 样本数量
 * @param num_buckets 直方图桶数
 * @param histogram 返回的直方图数据
 * @return 0 成功，-1 失败
 */
int cxl_analysis_histogram(const uint64_t *timings, int num_samples,
                          int num_buckets, uint32_t *histogram);

/**
 * @brief 生成时间分布可视化（文本格式）
 * @param timings 时间数据数组
 * @param num_samples 样本数量
 * @param width 输出宽度（字符）
 * @param height 输出高度（行）
 * @return 生成的文本字符串（需要由调用者释放）
 */
char *cxl_analysis_ascii_plot(const uint64_t *timings, int num_samples,
                              int width, int height);

/**
 * @brief 生成侧信道攻击成功率报告
 * @param results 攻击结果数组
 * @param num_results 结果数量
 * @param num_data_placement_types 数据放置类型数量
 * @param num_thread_configs 线程配置数量
 * @param output_file 输出报告文件路径
 * @return 0 成功，-1 失败
 */
int cxl_analysis_attack_success_report(const attack_result_t *results,
                                       int num_results,
                                       int num_data_placement_types,
                                       int num_thread_configs,
                                       const char *output_file);

/**
 * @brief 分析 CXL Memory 与普通内存的延迟差异
 * @param cxl_timings CXL 内存访问时间数组
 * @param normal_timings 普通内存访问时间数组
 * @param num_samples 样本数量
 * @param latency_diff 返回延迟差异
 * @param signal_strength 返回可观测信号强度
 * @return 0 成功，-1 失败
 */
int cxl_analysis_latency_difference(const uint64_t *cxl_timings,
                                    const uint64_t *normal_timings,
                                    int num_samples,
                                    double *latency_diff,
                                    double *signal_strength);

/**
 * @brief 分析命中与未命中的时间分布分离度
 * @param hit_timings 命中时间样本数组
 * @param num_hits 命中样本数
 * @param miss_timings 未命中时间样本数组
 * @param num_misses 未命中样本数
 * @param separation_ratio 返回分离度（0-1，越接近1分离度越好）
 * @return 0 成功，-1 失败
 */
int cxl_analysis_hit_miss_separation(const uint64_t *hit_timings, int num_hits,
                                     const uint64_t *miss_timings, int num_misses,
                                     double *separation_ratio);

/**
 * @brief 自动推荐最优测时阈值
 * @param hit_timings 命中时间样本
 * @param num_hits 命中样本数
 * @param miss_timings 未命中时间样本
 * @param num_misses 未命中样本数
 * @return 推荐的阈值，失败返回 0
 */
uint64_t cxl_analysis_recommend_threshold(const uint64_t *hit_timings, int num_hits,
                                          const uint64_t *miss_timings, int num_misses);

/**
 * @brief 从噪声中恢复信号（去噪分析）
 * @param raw_samples 原始样本数组
 * @param num_samples 样本数量
 * @param filter_type 滤波器类型（moving_average/median/kalman）
 * @param filtered_samples 返回的滤波样本数组
 * @return 0 成功，-1 失败
 */
int cxl_analysis_signal_recovery(const uint64_t *raw_samples, int num_samples,
                                 const char *filter_type, uint64_t *filtered_samples);

/**
 * @brief 生成可视化热图（用于多维数据）
 * @param data 二维数据矩阵（行优先存储）
 * @param rows 矩阵行数
 * @param cols 矩阵列数
 * @param output_file CSV 或 JSON 格式的输出文件
 * @return 0 成功，-1 失败
 */
int cxl_analysis_heatmap(const double *data, int rows, int cols,
                        const char *output_file);

/**
 * @brief 导出 JSON 格式的攻击分析结果
 * @param results 攻击结果数组
 * @param num_results 结果数量
 * @param output_file 输出 JSON 文件路径
 * @return 0 成功，-1 失败
 */
int cxl_analysis_export_json(const attack_result_t *results, int num_results,
                            const char *output_file);

/**
 * @brief 执行完整的分析流程并生成综合报告
 * @param results 攻击结果数据
 * @param num_results 结果数量
 * @param config 框架配置信息
 * @param output_dir 输出报告目录
 * @return 0 成功，-1 失败
 */
int cxl_analysis_full_report(const attack_result_t *results, int num_results,
                            const cxl_config_t *config, const char *output_dir);

/**
 * @brief 清理分析模块资源
 * @return 0 成功，-1 失败
 */
int cxl_analysis_cleanup(void);

/**
 * @brief 绘制攻击成功率曲线（随迭代次数）
 * @param success_rates 成功率数组（0-1 范围）
 * @param num_points 数据点数量
 * @param output_file 输出文件路径
 * @return 0 成功，-1 失败
 */
int cxl_analysis_plot_success_curve(const float *success_rates, int num_points,
                                   const char *output_file);

#endif /* CXL_ANALYSIS_H */
