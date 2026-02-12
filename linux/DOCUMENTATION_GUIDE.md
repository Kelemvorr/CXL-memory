# 文档导航指南

本项目采用**单一综合参考指南**设计，所有信息整合在一个文档中。

## 📚 文档清单

### 1. **COMPREHENSIVE_GUIDE.md** ⭐ 推荐首先阅读

**内容**: 完整的参考指南，包含以下所有信息
- ✅ 项目概述和快速开始
- ✅ 编译和安装说明
- ✅ **完整的命令行参数参考**（所有选项详解）
- ✅ **五大测试模式详细说明**（原理、使用、输出示例）
- ✅ **完整的函数参考**（7 个模块，50+ 个函数）
  - cxl_common.h - 通用工具
  - cxl_prepreparation.h - 初始化和配置
  - cxl_victim.h - 受害者操作
  - cxl_attacker.h - 攻击者操作
  - cxl_attack_primitives.h - 原始攻击操作
  - cxl_observation.h - 数据观测
  - cxl_analysis.h - 分析工具
- ✅ **5 个实际使用场景**（从快速检查到学术研究）
- ✅ **结果分析指南**（安全评级、延迟解读）
- ✅ **故障排除**（常见问题和解决方案）
- ✅ **高级用法**（批量测试、性能基准）
- ✅ **FAQ**（常见问题解答）

**访问方式**:
```bash
# 快速浏览
less COMPREHENSIVE_GUIDE.md

# 查找特定内容
grep -n "Flush + Reload" COMPREHENSIVE_GUIDE.md
```

**推荐用途**:
- ✅ 首次了解框架
- ✅ 查找任何命令或函数说明
- ✅ 了解如何使用特定功能
- ✅ 参加考试或研讨会
- ✅ 编写脚本或集成代码

---

### 2. **README.md** 简要说明

**内容**: 框架的简要介绍和基本概述
- 项目概述
- 系统要求
- 目录结构
- 编译和运行
- 基本概念

**推荐用途**:
- ✅ 了解项目整体情况
- ✅ 检查系统兼容性
- ✅ 快速开始编译

---

### 3. **PROJECT_SUMMARY.md** 架构说明

**内容**: 项目架构和设计说明
- 七个核心模块的设计理念
- 模块间的交互关系
- 数据流向
- 设计模式

**推荐用途**:
- ✅ 理解框架的整体架构
- ✅ 修改或扩展框架
- ✅ 学术研究背景

---

### 4. **API_REFERENCE.md** 快速 API 查询

**内容**: API 函数的简明参考（可选）
- 函数原型
- 参数说明
- 返回值

**推荐用途**:
- ✅ 快速查询函数签名
- ✅ C 语言编程参考

**注**: COMPREHENSIVE_GUIDE.md 已包含所有 API 信息，此文件为可选

---

## 🎯 快速导航

### 🚀 快速开始 (5 分钟)

1. 读 **README.md** - 了解项目
2. 运行 **make && ./bin/cxl_framework -m 4**
3. 查看结果

---

### 📖 学习框架 (30 分钟)

**COMPREHENSIVE_GUIDE.md**:
- 第 1-2 部分: 项目概述和快速开始
- 第 3 部分: 编译和安装
- 第 4-5 部分: 参数和测试模式

---

### 🔧 使用特定功能 (10-15 分钟)

查询 **COMPREHENSIVE_GUIDE.md** 的对应部分:
- 需要命令行参数? → **"命令行参数完全参考"**
- 想了解测试模式? → **"五大测试模式详解"**
- 需要函数说明? → **"完整函数参考"**
- 遇到问题? → **"故障排除"**

---

### 💡 实战应用 (根据场景)

参考 **COMPREHENSIVE_GUIDE.md** 中的 **"使用场景和示例"**:
- 快速风险评估
- 标准安全评估
- 详细漏洞分析
- 学术研究
- CI/CD 集成

---

### 🏗️ 修改或扩展框架 (2-3 小时)

1. 读 **PROJECT_SUMMARY.md** - 理解架构
2. 阅读 **COMPREHENSIVE_GUIDE.md** 的函数参考 - 理解各模块
3. 查看 `src/` 目录下的代码实现
4. 参考 `COMPREHENSIVE_GUIDE.md` 中的高级用法部分

---

## 📊 文档信息统计

| 文档 | 大小 | 内容 | 阅读时间 |
|------|------|------|---------|
| COMPREHENSIVE_GUIDE.md | ~800 KB | 完整参考 | 30-60 分钟 |
| README.md | ~50 KB | 简要说明 | 5-10 分钟 |
| PROJECT_SUMMARY.md | ~100 KB | 架构设计 | 10-15 分钟 |
| API_REFERENCE.md | ~30 KB | API 快速查询 | 5 分钟 |

---

## ✅ 迁移总结

### 之前 (五个分散的文档)
```
├── QUICKSTART.md              # 快速开始
├── MAIN_FUNCTION_GUIDE.md     # 主函数说明
├── TESTING_GUIDE.md           # 使用指南 (30+ 页)
├── OPTIMIZATION_SUMMARY.md    # 优化说明
└── OPTIMIZATION_REPORT.md     # 优化报告
```
**问题**:  多个文档内容重复，用户需翻阅多个文件

### 现在 (综合设计)
```
├── COMPREHENSIVE_GUIDE.md     # ⭐ 唯一的详细参考
├── README.md                  # 简要说明
├── PROJECT_SUMMARY.md         # 架构设计
└── API_REFERENCE.md          # 快速查询 (可选)
```
**优势**:  
- ✅ 信息集中，易于查找
- ✅ 减少内容重复
- ✅ 避免文档不同步
- ✅ 单一入口，清晰的导航结构

---

## 🎓 典型使用范例

### 场景 1: 我是新手，想快速了解框架

```
① 阅读 README.md (5 分钟)
   ↓
② 阅读 COMPREHENSIVE_GUIDE.md 前两部分 (10 分钟)
   ↓
③ 编译并运行: make && ./bin/cxl_framework -m 4 (5 分钟)
   ↓
④ 阅读 COMPREHENSIVE_GUIDE.md 的"结果分析指南"部分 (5 分钟)
   
总时间: ~25 分钟
```

### 场景 2: 我需要运行特定的测试

```
① COMPREHENSIVE_GUIDE.md → "命令行参数完全参考"
   ↓
② 构建命令: ./bin/cxl_framework [OPTIONS]
   ↓
③ 运行测试
   ↓
④ COMPREHENSIVE_GUIDE.md → "结果分析指南"
```

### 场景 3: 我需要理解代码实现

```
① 读 PROJECT_SUMMARY.md → 架构概览
   ↓
② COMPREHENSIVE_GUIDE.md → "完整函数参考"
   ↓
③ 查看 src/ 下的代码实现
   ↓
④ 根据需要参考 COMPREHENSIVE_GUIDE.md 的特定部分
```

---

## 🔗 快速链接

### 在 COMPREHENSIVE_GUIDE.md 中快速定位

```bash
# 查找命令行参数说明
grep -n "命令行参数完全参考" COMPREHENSIVE_GUIDE.md

# 查找特定模式说明
grep -n "模式 0:" COMPREHENSIVE_GUIDE.md

# 查找函数说明
grep -n "cxl_attacker_flush_reload" COMPREHENSIVE_GUIDE.md

# 查找故障排除
grep -n "故障排除" COMPREHENSIVE_GUIDE.md
```

---

## 📝 文档维护

所有信息现已整合到 **COMPREHENSIVE_GUIDE.md** 中。未来的更新应该：

1. ✅ 编辑 **COMPREHENSIVE_GUIDE.md**（唯一的详细参考）
2. ✅ 如需更新基本信息，编辑 **README.md**
3. ✅ 如需架构修改，编辑 **PROJECT_SUMMARY.md**

这种设计避免了多个文档之间的同步问题。

---

## 💬 获得帮助

### 常见问题

**Q: 我应该从哪里开始？**  
A: 从 **README.md** 开始，然后打开 **COMPREHENSIVE_GUIDE.md**

**Q: 如何找到特定功能的说明？**  
A: 在 **COMPREHENSIVE_GUIDE.md** 中使用搜索（Ctrl+F）

**Q: 如何编译项目？**  
A: 查看 **README.md** 或 **COMPREHENSIVE_GUIDE.md** 的"编译和安装"部分

**Q: 如何运行测试？**  
A: **COMPREHENSIVE_GUIDE.md** → "快速开始" 或 "使用场景和示例"

**Q: 如何理解结果？**  
A: **COMPREHENSIVE_GUIDE.md** → "结果分析指南"

**Q: 我遇到了错误？**  
A: **COMPREHENSIVE_GUIDE.md** → "故障排除"

---

**文档整合完成日期**: 2026-02-11  
**状态**: ✅ 就绪  
**推荐阅读**: COMPREHENSIVE_GUIDE.md
