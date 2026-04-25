#!/bin/bash

echo "Building RC4 + Poly1305 WebAssembly module..."

if ! command -v emcc &> /dev/null; then
    echo "Error: emcc (Emscripten) not found in PATH"
    echo "Please install Emscripten and source the environment"
    echo "See: https://emscripten.org/docs/getting_started/downloads.html"
    exit 1
fi

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
ROOT_DIR=$(cd "$SCRIPT_DIR/../.." &>/dev/null && pwd)

echo "Source directory: $ROOT_DIR"
echo "Output directory: $SCRIPT_DIR"

emcc \
    -O3 \
    -s WASM=1 \
    -s EXPORTED_FUNCTIONS='["_aead_rc4_encrypt_wasm", "_aead_rc4_decrypt_wasm", "_arcfour_encrypt_wasm", "_free_wasm", "_generate_nonce_wasm"]' \
    -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "malloc", "free"]' \
    -s ALLOW_MEMORY_GROWTH=1 \
    -I"$ROOT_DIR/include" \
    "$ROOT_DIR/src/arcfour.c" \
    "$ROOT_DIR/src/poly1305.c" \
    "$ROOT_DIR/src/aead_rc4.c" \
    "$SCRIPT_DIR/arcfour_wasm.c" \
    -o "$SCRIPT_DIR/arcfour.js" \
    --pre-js "$SCRIPT_DIR/preamble.js"

if [ $? -eq 0 ]; then
    echo "✅ WebAssembly build completed successfully!"
    echo "Output files:"
    echo "  - $SCRIPT_DIR/arcfour.js"
    echo "  - $SCRIPT_DIR/arcfour.wasm"
    echo ""
    echo "To test, serve the directory with a web server:"
    echo "  cd $SCRIPT_DIR"
    echo "  python -m http.server 8000"
    echo "  Then open http://localhost:8000 in your browser"
else
    echo "❌ WebAssembly build failed!"
    exit 1
fi