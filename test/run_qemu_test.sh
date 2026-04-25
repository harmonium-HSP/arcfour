#!/bin/bash

# QEMU Cortex-M3 测试脚本
# 需要安装: sudo apt install qemu-system-arm gcc-arm-none-eabi

set -e

echo "=== QEMU ARM Cortex-M Simulation ==="

# 检查工具是否安装
if ! command -v arm-none-eabi-gcc &> /dev/null; then
    echo "❌ arm-none-eabi-gcc not found"
    echo "请安装: sudo apt install gcc-arm-none-eabi"
    exit 1
fi

if ! command -v qemu-system-arm &> /dev/null; then
    echo "❌ qemu-system-arm not found"
    echo "请安装: sudo apt install qemu-system-arm"
    exit 1
fi

echo "✅ Tools ready"

# 编译测试程序
echo ""
echo "Compiling test program..."
arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb \
    -I../include \
    -DARCFOUR_ARM_OPT=1 \
    qemu_test.c \
    ../src/arm/arcfour_arm.c \
    ../src/arcfour_key.c \
    -o qemu_test.elf \
    -specs=rdimon.specs \
    -lrdimon

if [ $? -eq 0 ]; then
    echo "✅ Compilation successful"
else
    echo "❌ Compilation failed"
    exit 1
fi

# 在 QEMU 中运行
echo ""
echo "Running in QEMU..."
echo "========================================"

qemu-system-arm -machine lm3s6965evb \
    -kernel qemu_test.elf \
    -nographic \
    -semihosting

echo "========================================"
echo ""
echo "=== Test completed ==="