# ====== CXL Memory Side-Channel Detection Framework Makefile ======

CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -O2 -fPIC
LDFLAGS := -lnuma -lm -lpthread

# 目录定义
SRC_DIR := src
INC_DIR := include
OBJ_DIR := obj
BIN_DIR := bin
OUTPUT_DIR := results

# 源文件
SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SOURCES))
HEADERS := $(wildcard $(INC_DIR)/*.h)

# 目标
TARGET := $(BIN_DIR)/cxl_framework

# 默认目标
.PHONY: all clean run help setup

all: setup $(TARGET)

# 设置目录
setup:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OUTPUT_DIR)
	@echo "[INFO] Directories created"

# 编译目标
$(TARGET): $(OBJECTS)
	@echo "[BUILD] Linking $@"
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ $^ $(LDFLAGS)
	@echo "[SUCCESS] Framework compiled: $@"

# 编译对象文件
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@echo "[BUILD] Compiling $<"
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# 运行框架
run: all
	@echo "[RUN] Executing CXL Framework"
	@./$(TARGET)

# 清理编译产物
clean:
	@echo "[CLEAN] Removing build artifacts"
	@rm -rf $(OBJ_DIR) $(BIN_DIR)
	@rm -rf $(OUTPUT_DIR)/*.csv $(OUTPUT_DIR)/*.json $(OUTPUT_DIR)/*.txt
	@echo "[SUCCESS] Cleanup completed"

# 完全清理
distclean: clean
	@echo "[CLEAN] Removing all generated files"
	@rm -rf $(OUTPUT_DIR)
	@echo "[SUCCESS] Full cleanup completed"

# 帮助信息
help:
	@echo "======== CXL Framework Build Help ========"
	@echo "Targets:"
	@echo "  make              - Build the framework (default)"
	@echo "  make run          - Build and run the framework"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make distclean    - Remove all generated files"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Configuration:"
	@echo "  CC:       $(CC)"
	@echo "  CFLAGS:   $(CFLAGS)"
	@echo "  LDFLAGS:  $(LDFLAGS)"
	@echo ""
	@echo "Directories:"
	@echo "  Source:   $(SRC_DIR)"
	@echo "  Headers:  $(INC_DIR)"
	@echo "  Objects:  $(OBJ_DIR)"
	@echo "  Binary:   $(BIN_DIR)"
	@echo "  Output:   $(OUTPUT_DIR)"
	@echo "=========================================="

# 显示编译信息
.PHONY: info
info:
	@echo "[INFO] Build Information"
	@echo "Source files:"
	@echo $(SOURCES) | sed 's/ /\n  /g'
	@echo ""
	@echo "Header files:"
	@echo $(HEADERS) | sed 's/ /\n  /g'
	@echo ""
	@echo "Object files to create:"
	@echo $(OBJECTS) | sed 's/ /\n  /g'

# 编译统计
.PHONY: stats
stats:
	@echo "[STATS] Code Statistics"
	@echo "Total lines of code (source):"
	@wc -l $(SOURCES) | tail -1
	@echo "Total lines of code (headers):"
	@wc -l $(HEADERS) | tail -1
	@echo "Total lines of code (all):"
	@wc -l $(SOURCES) $(HEADERS) | tail -1
