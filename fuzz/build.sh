#!/bin/bash
#
# OSS-Fuzz build script for arcfour library
#
# This script is invoked by OSS-Fuzz to build the fuzz target.
# See: https://github.com/google/oss-fuzz
#

set -eux

# Change to source directory (OSS-Fuzz environment)
cd "$SRC/arcfour"

# Compiler settings for fuzzing
CC="${CC:-clang}"
CXX="${CXX:-clang++}"
CFLAGS="${CFLAGS:-}"
CXXFLAGS="${CXXFLAGS:-}"

# Add sanitizers and fuzzer flags
COMMON_FLAGS="-fsanitize=fuzzer-no-link,address,undefined -O1 -g"
COMMON_FLAGS+=" -fno-omit-frame-pointer -fno-optimize-sibling-calls"
COMMON_FLAGS+=" -Wall -Wextra"

# Source files
SRCS="src/arcfour.c src/arcfour_static.c src/aead_rc4.c src/arcfour_utils.c"

# Include directories (relative to project root)
INCLUDES="-Iinclude -Isrc"

# Step 1: Compile all source files to object files
echo "Compiling source files..."
for src in $SRCS; do
    $CC $COMMON_FLAGS $CFLAGS $INCLUDES -c "$src" -o "${src%.c}.o"
done

# Step 2: Create static library
echo "Creating static library..."
ar rcs libarcfour.a src/*.o

# Step 3: Compile fuzz target
echo "Compiling fuzz target..."
$CXX $COMMON_FLAGS $CXXFLAGS $INCLUDES \
    fuzz/fuzz_arcfour.cc \
    libarcfour.a \
    $LIB_FUZZING_ENGINE \
    -o "$OUT/fuzz_arcfour"

# Step 4: Copy dictionary file if it exists
echo "Copying dictionary..."
if [ -f "fuzz/fuzz_arcfour.dict" ]; then
    cp "fuzz/fuzz_arcfour.dict" "$OUT/fuzz_arcfour.dict"
fi

# Step 5: Copy seed corpus if it exists
echo "Copying seed corpus..."
if [ -d "fuzz/corpus" ]; then
    zip -r "$OUT/fuzz_arcfour_seed_corpus.zip" "fuzz/corpus"
fi

echo "Build completed successfully!"
