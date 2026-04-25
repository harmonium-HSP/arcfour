# RC4 + Poly1305 Bindings

This directory contains language bindings and WebAssembly support for the RC4+Poly1305 encryption library.

## 📁 Directory Structure

```
bindings/
├── python/          # Python bindings using CFFI
│   ├── __init__.py
│   ├── arcfour.py
│   ├── arcfour_cffi.py
│   ├── setup.py
│   └── test_arcfour.py
└── wasm/            # WebAssembly support
    ├── arcfour_wasm.c
    ├── index.html
    ├── build.sh
    └── preamble.js
```

---

## 🐍 Python Bindings

### Requirements

- Python 3.6+
- `cffi` package
- Pre-built C library (`libarcfour.dll` / `libarcfour.so` / `libarcfour.dylib`)

### Installation

1. First build the C library:
   ```bash
   mkdir build && cd build
   cmake -DBUILD_AEAD_RC4=ON ..
   cmake --build .
   ```

2. Install the Python package:
   ```bash
   cd bindings/python
   pip install .
   ```

### Usage

```python
from arcfour import ArcFourAEAD

# Generate a random key (32 bytes recommended)
key = ArcFourAEAD.generate_key()

# Create AEAD instance
aead = ArcFourAEAD(key)

# Encrypt
nonce, ciphertext, tag = aead.encrypt(b"Secret message")

# Decrypt
decrypted = aead.decrypt(nonce, ciphertext, tag)
```

### Running Tests

```bash
cd bindings/python
python -m unittest test_arcfour.py
```

---

## 🌐 WebAssembly

### Requirements

- Emscripten SDK (emcc compiler)
- Node.js (for testing)

### Installing Emscripten

**Linux/macOS:**
```bash
# Install using emsdk
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh  # Add to .bashrc or .zshrc
```

**Windows:**
```powershell
# Install using emsdk
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
emsdk install latest
emsdk activate latest
emsdk_env.bat  # Add to PATH
```

### Building

**Option 1: Using build.sh (Linux/macOS)**
```bash
cd bindings/wasm
chmod +x build.sh
./build.sh
```

**Option 2: Using CMake**
```bash
mkdir build && cd build
cmake -DBUILD_WASM=ON ..
make
```

**Option 3: Manual emcc command**
```bash
emcc -O3 -s WASM=1 \
    -s EXPORTED_FUNCTIONS='["_aead_rc4_encrypt_wasm", "_aead_rc4_decrypt_wasm", "_arcfour_encrypt_wasm", "_free_wasm", "_generate_nonce_wasm"]' \
    -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "malloc", "free"]' \
    -s ALLOW_MEMORY_GROWTH=1 \
    -I../../include \
    ../../src/arcfour.c \
    ../../src/poly1305.c \
    ../../src/aead_rc4.c \
    arcfour_wasm.c \
    -o arcfour.js \
    --pre-js preamble.js
```

### Testing

1. Serve the directory with a web server:
   ```bash
   cd bindings/wasm
   python -m http.server 8000
   ```

2. Open `http://localhost:8000` in your browser

### Usage (JavaScript)

```javascript
// Wait for WASM to load
Module.onRuntimeInitialized = function() {
    // Encrypt
    const resultPtr = Module._aead_rc4_encrypt_wasm(
        keyPtr, keyLength,
        noncePtr, nonceLength,
        aadPtr, aadLength,
        plaintextPtr, plaintextLength,
        outputLenPtr
    );
    
    // Decrypt
    const plaintextPtr = Module._aead_rc4_decrypt_wasm(
        keyPtr, keyLength,
        noncePtr, nonceLength,
        aadPtr, aadLength,
        ciphertextPtr, ciphertextLength,
        tagPtr,
        outputLenPtr
    );
    
    // Free memory
    Module._free_wasm(resultPtr);
};
```

---

## ⚠️ Troubleshooting

### Python: "Could not load arcfour library"

Ensure the C library is built and accessible:
```bash
# Linux/macOS
export LD_LIBRARY_PATH=/path/to/build:$LD_LIBRARY_PATH

# Windows
set PATH=C:\path\to\build;%PATH%
```

### WASM: "emcc command not found"

Activate the Emscripten environment:
```bash
# Linux/macOS
source /path/to/emsdk/emsdk_env.sh

# Windows
C:\path\to\emsdk\emsdk_env.bat
```

### IDE Warning: "'emscripten.h' file not found"

This is normal if Emscripten is not installed or not configured in your IDE. The code will still compile correctly when using emcc.

To fix in VS Code, add to `.vscode/c_cpp_properties.json`:
```json
{
    "configurations": [{
        "includePath": [
            "${workspaceFolder}/include",
            "/path/to/emsdk/upstream/emscripten/system/include"
        ]
    }]
}
```
