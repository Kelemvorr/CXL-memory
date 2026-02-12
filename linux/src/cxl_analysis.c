#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include "cxl_analysis.h"
#include "cxl_common.h"

/* ====== 分析模块状态 ====== */
static struct {
    char output_dir[256];
    int initialized;
} analysis_state = {0};

/* ====== 初始化与清理 ====== */
int cxl_analysis_init(const char *output_dir) {
    if (!output_dir) {
        fprintf(stderr, "[ERROR] Invalid output directory\n");
        return -1;
    }
    
    strncpy(analysis_state.output_dir, output_dir, sizeof(analysis_state.output_dir) - 1);
    analysis_state.initialized = 1;
    
    /* 创建输出目录 */
    char mkdir_cmd[512];
    snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p %s", output_dir);
    system(mkdir_cmd);
    
    fprintf(stdout, "[INFO] Analysis module initialized with output directory: %s\n", output_dir);
    
    return 0;
}

int cxl_analysis_cleanup(void) {
    analysis_state.initialized = 0;
    
    fprintf(stdout, "[INFO] Analysis module cleanup completed\n");
    
    return 0;
}

/* ====== 基本统计 ====== */
int cxl_analysis_compute_statistics(const uint64_t *timings, int num_samples,
                                    uint64_t *min, uint64_t *max, 
                                    double *mean, double *median, double *stddev) {
    if (!timings || num_samples <= 0 || !min || !max || !mean || !median || !stddev) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    *min = timings[0];
    *max = timings[0];
    *mean = 0.0;
    
    /* 计算最小值、最大值和平均值 */
    for (int i = 0; i < num_samples; i++) {
        if (timings[i] < *min) *min = timings[i];
        if (timings[i] > *max) *max = timings[i];
        *mean += timings[i];
    }
    *mean /= num_samples;
    
    /* 计算中位数 */
    uint64_t *sorted = malloc(num_samples * sizeof(uint64_t));
    memcpy(sorted, timings, num_samples * sizeof(uint64_t));
    
    for (int i = 0; i < num_samples; i++) {
        for (int j = i + 1; j < num_samples; j++) {
            if (sorted[i] > sorted[j]) {
                uint64_t tmp = sorted[i];
                sorted[i] = sorted[j];
                sorted[j] = tmp;
            }
        }
    }
    
    if (num_samples % 2 == 0) {
        *median = (sorted[num_samples / 2 - 1] + sorted[num_samples / 2]) / 2.0;
    } else {
        *median = sorted[num_samples / 2];
    }
    
    /* 计算标准差 */
    double variance = 0.0;
    for (int i = 0; i < num_samples; i++) {
        double diff = timings[i] - *mean;
        variance += diff * diff;
    }
    variance /= num_samples;
    *stddev = sqrt(variance);
    
    free(sorted);
    
    return 0;
}

/* ====== 分布对比 ====== */
int cxl_analysis_compare_distributions(const uint64_t *timings_a, int num_a,
                                       const uint64_t *timings_b, int num_b,
                                       double *difference, double *p_value) {
    if (!timings_a || num_a <= 0 || !timings_b || num_b <= 0) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    /* 计算平均值 */
    double mean_a = 0.0, mean_b = 0.0;
    for (int i = 0; i < num_a; i++) mean_a += timings_a[i];
    for (int i = 0; i < num_b; i++) mean_b += timings_b[i];
    mean_a /= num_a;
    mean_b /= num_b;
    
    *difference = mean_a - mean_b;
    
    /* 简化的 t-test p 值计算 */
    double var_a = 0.0, var_b = 0.0;
    for (int i = 0; i < num_a; i++) {
        double diff = timings_a[i] - mean_a;
        var_a += diff * diff;
    }
    for (int i = 0; i < num_b; i++) {
        double diff = timings_b[i] - mean_b;
        var_b += diff * diff;
    }
    var_a /= (num_a - 1);
    var_b /= (num_b - 1);
    
    double t_stat = *difference / sqrt((var_a / num_a) + (var_b / num_b));
    
    /* 简化的 p 值估计 */
    *p_value = 1.0 / (1.0 + fabs(t_stat));
    
    return 0;
}

/* ====== CSV 导出 ====== */
int cxl_analysis_export_csv(const uint64_t *timings, int num_samples,
                            const char *label, const char *output_file) {
    if (!timings || num_samples <= 0 || !label || !output_file) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    FILE *file = fopen(output_file, "w");
    if (!file) {
        fprintf(stderr, "[ERROR] Failed to open output file: %s\n", output_file);
        return -1;
    }
    
    fprintf(file, "sample_id,%s\n", label);
    
    for (int i = 0; i < num_samples; i++) {
        fprintf(file, "%d,%lu\n", i, timings[i]);
    }
    
    fclose(file);
    
    fprintf(stdout, "[INFO] CSV exported to: %s\n", output_file);
    
    return 0;
}

/* ====== 直方图 ====== */
int cxl_analysis_histogram(const uint64_t *timings, int num_samples,
                          int num_buckets, uint32_t *histogram) {
    if (!timings || num_samples <= 0 || num_buckets <= 0 || !histogram) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    uint64_t min = timings[0], max = timings[0];
    for (int i = 0; i < num_samples; i++) {
        if (timings[i] < min) min = timings[i];
        if (timings[i] > max) max = timings[i];
    }
    
    memset(histogram, 0, num_buckets * sizeof(uint32_t));
    
    double range = (max - min + 1.0);
    double bucket_size = range / num_buckets;
    
    for (int i = 0; i < num_samples; i++) {
        int bucket = (int)((timings[i] - min) / bucket_size);
        if (bucket >= num_buckets) bucket = num_buckets - 1;
        if (bucket < 0) bucket = 0;
        
        histogram[bucket]++;
    }
    
    return 0;
}

/* ====== ASCII 绘图 ====== */
char *cxl_analysis_ascii_plot(const uint64_t *timings, int num_samples,
                              int width, int height) {
    if (!timings || num_samples <= 0 || width <= 0 || height <= 0) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return NULL;
    }
    
    /* 简化实现：返回一个简单的字符串表示 */
    char *plot = malloc(width * height * 2);
    memset(plot, ' ', width * height * 2);
    
    fprintf(stdout, "[INFO] ASCII plot generated\n");
    
    return plot;
}

/* ====== 攻击成功率报告 ====== */
int cxl_analysis_attack_success_report(const attack_result_t *results,
                                       int num_results,
                                       int num_data_placement_types,
                                       int num_thread_configs,
                                       const char *output_file) {
    if (!results || num_results <= 0 || !output_file) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    FILE *file = fopen(output_file, "w");
    if (!file) {
        fprintf(stderr, "[ERROR] Failed to open output file: %s\n", output_file);
        return -1;
    }
    
    fprintf(file, "Attack Success Rate Report\n");
    fprintf(file, "==========================\n\n");
    
    /* 计算总体成功率 */
    int successful = 0;
    for (int i = 0; i < num_results; i++) {
        if (results[i].is_hit) successful++;
    }
    
    float overall_rate = (float)successful / num_results;
    fprintf(file, "Overall Success Rate: %.2f%%\n\n", overall_rate * 100.0f);
    
    /* 按数据放置类型分析 */
    fprintf(file, "Success Rate by Data Placement:\n");
    for (int placement = 0; placement < num_data_placement_types; placement++) {
        int count = 0, hits = 0;
        
        for (int i = 0; i < num_results; i++) {
            if (results[i].data_location == placement) {
                count++;
                if (results[i].is_hit) hits++;
            }
        }
        
        if (count > 0) {
            float rate = (float)hits / count;
            fprintf(file, "  Placement %d: %.2f%% (%d/%d)\n", 
                   placement, rate * 100.0f, hits, count);
        }
    }
    
    /* 统计数据 */
    fprintf(file, "\nDetailed Statistics:\n");
    fprintf(file, "Total Attacks: %d\n", num_results);
    fprintf(file, "Successful: %d\n", successful);
    fprintf(file, "Failed: %d\n", num_results - successful);
    
    fclose(file);
    
    fprintf(stdout, "[INFO] Attack report generated: %s\n", output_file);
    
    return 0;
}

/* ====== CXL 延迟分析 ====== */
int cxl_analysis_latency_difference(const uint64_t *cxl_timings,
                                    const uint64_t *normal_timings,
                                    int num_samples,
                                    double *latency_diff,
                                    double *signal_strength) {
    if (!cxl_timings || !normal_timings || num_samples <= 0) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    /* 计算平均延迟 */
    double cxl_avg = 0.0, normal_avg = 0.0;
    for (int i = 0; i < num_samples; i++) {
        cxl_avg += cxl_timings[i];
        normal_avg += normal_timings[i];
    }
    cxl_avg /= num_samples;
    normal_avg /= num_samples;
    
    *latency_diff = cxl_avg - normal_avg;
    
    /* 计算信噪比 */
    double cxl_var = 0.0, normal_var = 0.0;
    for (int i = 0; i < num_samples; i++) {
        double diff = cxl_timings[i] - cxl_avg;
        cxl_var += diff * diff;
        diff = normal_timings[i] - normal_avg;
        normal_var += diff * diff;
    }
    cxl_var /= num_samples;
    normal_var /= num_samples;
    
    double signal = fabs(*latency_diff);
    double noise = sqrt((cxl_var + normal_var) / 2.0);
    
    *signal_strength = signal / (noise + 1e-9);
    
    return 0;
}

/* ====== 命中/未命中分离度 ====== */
int cxl_analysis_hit_miss_separation(const uint64_t *hit_timings, int num_hits,
                                     const uint64_t *miss_timings, int num_misses,
                                     double *separation_ratio) {
    if (!hit_timings || !miss_timings || num_hits <= 0 || num_misses <= 0) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    double hit_mean = 0.0, miss_mean = 0.0;
    for (int i = 0; i < num_hits; i++) hit_mean += hit_timings[i];
    for (int i = 0; i < num_misses; i++) miss_mean += miss_timings[i];
    hit_mean /= num_hits;
    miss_mean /= num_misses;
    
    double hit_var = 0.0, miss_var = 0.0;
    for (int i = 0; i < num_hits; i++) {
        double diff = hit_timings[i] - hit_mean;
        hit_var += diff * diff;
    }
    for (int i = 0; i < num_misses; i++) {
        double diff = miss_timings[i] - miss_mean;
        miss_var += diff * diff;
    }
    hit_var /= num_hits;
    miss_var /= num_misses;
    
    double numerator = fabs(hit_mean - miss_mean);
    double denominator = sqrt(hit_var + miss_var);
    
    *separation_ratio = numerator / (denominator + 1e-9);
    
    /* 归一化到 0-1 范围 */
    *separation_ratio = 1.0 / (1.0 + (1.0 / *separation_ratio));
    
    return 0;
}

/* ====== 推荐阈值 ====== */
uint64_t cxl_analysis_recommend_threshold(const uint64_t *hit_timings, int num_hits,
                                          const uint64_t *miss_timings, int num_misses) {
    if (!hit_timings || !miss_timings || num_hits <= 0 || num_misses <= 0) {
        return 0;
    }
    
    /* 计算平均值 */
    double hit_avg = 0.0, miss_avg = 0.0;
    for (int i = 0; i < num_hits; i++) hit_avg += hit_timings[i];
    for (int i = 0; i < num_misses; i++) miss_avg += miss_timings[i];
    hit_avg /= num_hits;
    miss_avg /= num_misses;
    
    /* 推荐阈值为两者的中点 */
    uint64_t threshold = (uint64_t)((hit_avg + miss_avg) / 2.0);
    
    fprintf(stdout, "[INFO] Recommended timing threshold: %lu cycles\n", threshold);
    
    return threshold;
}

/* ====== 信号恢复 ====== */
int cxl_analysis_signal_recovery(const uint64_t *raw_samples, int num_samples,
                                 const char *filter_type, uint64_t *filtered_samples) {
    if (!raw_samples || !filtered_samples || num_samples <= 0 || !filter_type) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    if (strcmp(filter_type, "moving_average") == 0) {
        /* 移动平均滤波器 */
        int window = 5;
        
        for (int i = 0; i < num_samples; i++) {
            if (i < window / 2) {
                filtered_samples[i] = raw_samples[i];
            } else {
                uint64_t sum = 0;
                for (int j = i - window / 2; j <= i + window / 2 && j < num_samples; j++) {
                    sum += raw_samples[j];
                }
                filtered_samples[i] = sum / window;
            }
        }
    } else if (strcmp(filter_type, "median") == 0) {
        /* 中位数滤波器 */
        for (int i = 0; i < num_samples; i++) {
            if (i == 0 || i == num_samples - 1) {
                filtered_samples[i] = raw_samples[i];
            } else {
                uint64_t vals[3] = {raw_samples[i-1], raw_samples[i], raw_samples[i+1]};
                
                /* 简单排序 */
                for (int j = 0; j < 3; j++) {
                    for (int k = j + 1; k < 3; k++) {
                        if (vals[j] > vals[k]) {
                            uint64_t tmp = vals[j];
                            vals[j] = vals[k];
                            vals[k] = tmp;
                        }
                    }
                }
                
                filtered_samples[i] = vals[1];
            }
        }
    } else {
        /* 默认：无滤波 */
        memcpy(filtered_samples, raw_samples, num_samples * sizeof(uint64_t));
    }
    
    return 0;
}

/* ====== 热图 ====== */
int cxl_analysis_heatmap(const double *data, int rows, int cols,
                        const char *output_file) {
    if (!data || rows <= 0 || cols <= 0 || !output_file) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    FILE *file = fopen(output_file, "w");
    if (!file) {
        fprintf(stderr, "[ERROR] Failed to open output file: %s\n", output_file);
        return -1;
    }
    
    /* 写入 CSV 格式的热图数据 */
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            fprintf(file, "%.2f", data[i * cols + j]);
            if (j < cols - 1) fprintf(file, ",");
        }
        fprintf(file, "\n");
    }
    
    fclose(file);
    
    fprintf(stdout, "[INFO] Heatmap generated: %s\n", output_file);
    
    return 0;
}

/* ====== JSON 导出 ====== */
int cxl_analysis_export_json(const attack_result_t *results, int num_results,
                            const char *output_file) {
    if (!results || num_results <= 0 || !output_file) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    FILE *file = fopen(output_file, "w");
    if (!file) {
        fprintf(stderr, "[ERROR] Failed to open output file: %s\n", output_file);
        return -1;
    }
    
    fprintf(file, "{\n  \"results\": [\n");
    
    for (int i = 0; i < num_results; i++) {
        fprintf(file, "    {\n");
        fprintf(file, "      \"attack_id\": %lu,\n", results[i].attack_id);
        fprintf(file, "      \"is_hit\": %d,\n", results[i].is_hit);
        fprintf(file, "      \"latency_diff\": %lu,\n", results[i].latency_diff);
        fprintf(file, "      \"hit_count\": %u,\n", results[i].hit_count);
        fprintf(file, "      \"miss_count\": %u\n", results[i].miss_count);
        fprintf(file, "    }");
        
        if (i < num_results - 1) fprintf(file, ",");
        fprintf(file, "\n");
    }
    
    fprintf(file, "  ]\n}\n");
    
    fclose(file);
    
    fprintf(stdout, "[INFO] JSON report exported: %s\n", output_file);
    
    return 0;
}

/* ====== 完整报告 ====== */
int cxl_analysis_full_report(const attack_result_t *results, int num_results,
                            const cxl_config_t *config, const char *output_dir) {
    if (!results || num_results <= 0 || !config || !output_dir) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    char filepath[512];
    
    /* 生成各种报告 */
    snprintf(filepath, sizeof(filepath), "%s/attack_report.txt", output_dir);
    cxl_analysis_attack_success_report(results, num_results, 3, 3, filepath);
    
    snprintf(filepath, sizeof(filepath), "%s/results.json", output_dir);
    cxl_analysis_export_json(results, num_results, filepath);
    
    fprintf(stdout, "[INFO] Full report generated in: %s\n", output_dir);
    
    return 0;
}

/* ====== 成功率曲线 ====== */
int cxl_analysis_plot_success_curve(const float *success_rates, int num_points,
                                   const char *output_file) {
    if (!success_rates || num_points <= 0 || !output_file) {
        fprintf(stderr, "[ERROR] Invalid parameters\n");
        return -1;
    }
    
    FILE *file = fopen(output_file, "w");
    if (!file) {
        fprintf(stderr, "[ERROR] Failed to open output file: %s\n", output_file);
        return -1;
    }
    
    fprintf(file, "iteration,success_rate\n");
    
    for (int i = 0; i < num_points; i++) {
        fprintf(file, "%d,%.4f\n", i, success_rates[i]);
    }
    
    fclose(file);
    
    fprintf(stdout, "[INFO] Success curve plotted: %s\n", output_file);
    
    return 0;
}
