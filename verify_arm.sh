#!/bin/bash

echo "=== ARC4 ARM Optimization Verification ==="
echo ""

# 1. 检查编译器
if ! command -v arm-none-eabi-gcc &> /dev/null; then
    echo "❌ arm-none-eabi-gcc not found"
    echo "请安装: sudo apt install gcc-arm-none-eabi"
    exit 1
fi
echo "✅ Compiler found: $(arm-none-eabi-gcc --version | head -1)"

# 2. 创建临时目录
mkdir -p test_build

# 3. 编译 ARM 优化版本
echo ""
echo "Compiling ARM optimized version..."
arm-none-eabi-gcc -c -mcpu=cortex-m4 -mthumb \
    -Iinclude \
    -DARCFOUR_ARM_OPT=1 \
    src/arm/arcfour_arm.c \
    -o test_build/arcfour_arm.o \
    -O3

if [ $? -eq 0 ]; then
    echo "✅ Compilation successful"
else
    echo "❌ Compilation failed"
    exit 1
fi

# 4. 编译通用版本
echo ""
echo "Compiling generic version..."
arm-none-eabi-gcc -c -mcpu=cortex-m4 -mthumb \
    -Iinclude \
    src/common/arcfour_generic.c \
    -o test_build/arcfour_generic.o \
    -O3

if [ $? -eq 0 ]; then
    echo "✅ Generic compilation successful"
else
    echo "❌ Generic compilation failed"
    exit 1
fi

# 5. 检查代码大小
echo ""
echo "=== Code Size Comparison ==="
echo "ARM optimized version:"
arm-none-eabi-size test_build/arcfour_arm.o

ARM_SIZE=$(arm-none-eabi-size test_build/arcfour_arm.o | tail -1 | awk '{print $1}')

echo ""
echo "Generic version:"
arm-none-eabi-size test_build/arcfour_generic.o

GENERIC_SIZE=$(arm-none-eabi-size test_build/arcfour_generic.o | tail -1 | awk '{print $1}')

echo ""
if [ $ARM_SIZE -lt 2048 ]; then
    echo "✅ ARM code size: ${ARM_SIZE} bytes (<2KB)"
else
    echo "⚠️ ARM code size: ${ARM_SIZE} bytes (consider optimization)"
fi

# 6. 检查 ARM 指令
echo ""
echo "=== Instruction Check ==="
if arm-none-eabi-objdump -d test_build/arcfour_arm.o | grep -q "ldrb\|strb"; then
    echo "✅ Thumb byte instructions detected"
fi

if arm-none-eabi-objdump -d test_build/arcfour_arm.o | grep -q "ldr.*r\[.*\]"; then
    echo "✅ ARM load/store instructions detected"
fi

# 7. 清理临时文件
rm -rf test_build

echo ""
echo "=== Verification complete ==="
echo ""
echo "Summary:"
echo "- ARM optimized: ${ARM_SIZE} bytes"
echo "- Generic: ${GENERIC_SIZE} bytes"
echo "- Code size reduction: $((100 - ARM_SIZE * 100 / GENERIC_SIZE))%"