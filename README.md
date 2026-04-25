# ARCFOUR - Enhanced RC4 Cryptographic Library

![Build Status](https://github.com/harmonium-HSP/arcfour/actions/workflows/build.yml/badge.svg)
![Tests](https://github.com/harmonium-HSP/arcfour/actions/workflows/test.yml/badge.svg)
![Coverage](https://codecov.io/gh/harmonium-HSP/arcfour/branch/main/graph/badge.svg)
![License](https://img.shields.io/github/license/harmonium-HSP/arcfour)
![Version](https://img.shields.io/github/v/tag/harmonium-HSP/arcfour)

A C implementation of the RC4 stream cipher with enhanced security through initial keystream byte discarding.

## ⚠️ Security Warning

**This library is for educational and research purposes only!**

- RC4 algorithm has known theoretical security vulnerabilities
- Use **AES-GCM** or **ChaCha20-Poly1305** for production environments
- If using this library, ensure keys are >= 256-bit (32 bytes) of strong random data
- Never use with related keys (e.g., incrementing counters)

## Features

- ✅ Enhanced RC4 implementation with 50 million initial byte discard
- ✅ Static memory allocation support for embedded systems
- ✅ DMA-optimized implementation for high-performance scenarios
- ✅ Power-aware encryption API for battery-powered devices
- ✅ Interrupt-safe API for real-time systems
- ✅ Python bindings via CFFI
- ✅ WebAssembly support
- ✅ Cross-platform: Windows, Linux, macOS, ARM

## Building

### Prerequisites

| Tool | Minimum Version | Recommended Version | Purpose |
|------|-----------------|---------------------|---------|
| CMake | 3.16 | 3.22+ | Build system |
| GCC | 8.0 | 11.0+ | Linux/macOS compiler |
| Clang | 9.0 | 14.0+ | macOS/Linux compiler |
| MSVC | 2019 | 2022 | Windows compiler |
| Python | 3.6 | 3.10+ | Python bindings |
| Emscripten | 2.0 | 3.1+ | WebAssembly |
| Make | 4.0 | 4.3+ | Build tool |
| Git | 2.20 | 2.38+ | Version control |

### Build Options

You can use either CMake directly or the provided Makefile wrapper:

```bash
# Using Makefile (recommended)
make build        # Build the project
make test         # Run tests
make clean        # Clean build directory
make coverage     # Generate coverage report
make debug        # Build debug version

# Using CMake directly
mkdir build && cd build
cmake ..
make

# Build with all features
cmake -DARCFOUR_STATIC_ONLY=ON -DBUILD_DMA=ON -DBUILD_ISR_API=ON -DBUILD_POWER_API=ON ..
make
```

### CMake Options

| Option | Description | Default |
|--------|-------------|---------|
| `BUILD_SHARED_LIBS` | Build shared library | ON |
| `BUILD_STATIC_LIBS` | Build static library | ON |
| `ARCFOUR_STATIC_ONLY` | Static memory allocation only | OFF |
| `BUILD_DMA` | Enable DMA-optimized code | OFF |
| `BUILD_ISR_API` | Enable interrupt-safe API | OFF |
| `BUILD_POWER_API` | Enable power-aware API | OFF |
| `BUILD_TESTS` | Build test suite | ON |
| `BUILD_EXAMPLES` | Build example programs | ON |

## API Documentation

### Core Functions

```c
#include <arcfour.h>

// Initialize ARCFOUR context
arcfour_ctx* arcfour_init(const uint8_t* key, size_t key_len);

// Release ARCFOUR context
void arcfour_uninit(arcfour_ctx* ctx);

// Encrypt/decrypt data (RC4 is symmetric)
void arcfour_encrypt(arcfour_ctx* ctx, const uint8_t* plaintext, 
                     uint8_t* ciphertext, size_t len);
void arcfour_decrypt(arcfour_ctx* ctx, const uint8_t* ciphertext, 
                     uint8_t* plaintext, size_t len);
```

### Static Memory API (Embedded)

```c
#include <arcfour_static.h>

arcfour_ctx_t ctx;
arcfour_init_static(&ctx, key, key_len);
arcfour_encrypt_static(&ctx, plaintext, ciphertext, len);
```

### Power-Aware API

```c
#include <arcfour_power.h>

arcfour_power_hooks_t hooks = {
    .before_operation = power_up_callback,
    .after_operation = power_down_callback,
    .timeout_ms = 100
};

arcfour_ctx* ctx = arcfour_init_power_aware(key, key_len, &hooks);
arcfour_encrypt_power_aware(ctx, plaintext, ciphertext, len, &hooks);
```

## Usage Examples

### Basic Usage

```c
#include <stdio.h>
#include <string.h>
#include "arcfour.h"

int main() {
    uint8_t key[32];  // Use cryptographically secure random key
    // Fill key from secure source...
    
    arcfour_ctx* ctx = arcfour_init(key, 32);
    if (!ctx) {
        fprintf(stderr, "Initialization failed\n");
        return 1;
    }
    
    const char* message = "Hello, World!";
    uint8_t cipher[100];
    
    arcfour_encrypt(ctx, (const uint8_t*)message, cipher, strlen(message));
    
    printf("Encrypted successfully\n");
    
    arcfour_uninit(ctx);
    return 0;
}
```

### Python Bindings

```python
from arcfour import ARCFOUR

# Create instance with key
cipher = ARCFOUR(b'secret_key_32_bytes_long!!')

# Encrypt/decrypt
plaintext = b'Hello, World!'
ciphertext = cipher.encrypt(plaintext)
decrypted = cipher.decrypt(ciphertext)

assert plaintext == decrypted
```

## Project Structure

```
arcfour/
├── .github/workflows/     # CI/CD workflows
├── include/               # Public headers
│   ├── arcfour.h          # Core API
│   ├── arcfour_static.h   # Static memory API
│   ├── arcfour_dma.h      # DMA-optimized API
│   ├── arcfour_isr.h      # Interrupt-safe API
│   └── arcfour_power.h    # Power-aware API
├── src/                   # Source code
│   ├── arcfour.c          # Core implementation
│   ├── arcfour_static.c   # Static memory implementation
│   ├── arcfour_dma.c      # DMA-optimized implementation
│   ├── arcfour_isr.c      # ISR-safe implementation
│   ├── arcfour_power.c    # Power-aware implementation
│   └── arcfour_utils.c    # Utility functions
├── bindings/              # Language bindings
│   ├── python/            # Python CFFI bindings
│   └── wasm/              # WebAssembly support
├── test/                  # Test suite
├── example/               # Example programs
├── benchmark/             # Performance benchmarks
└── CMakeLists.txt         # Build configuration
```

## Testing

```bash
# Run all tests
cd build
ctest -v

# Run specific test
ctest -R test_power -v

# Generate coverage report (requires gcov)
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
make coverage
```

## Benchmarks

| Operation | Performance |
|-----------|-------------|
| Initialization | ~0.2-0.5 seconds (50M byte discard) |
| Encryption | ~100-200 MB/s |
| Memory per context | ~260 bytes |

## Security Features

- **Initial Keystream Discard**: 50 million bytes to mitigate FMS attack
- **32-bit Counter Wrap-around Protection**: Safe timestamp calculations
- **Integer Overflow Protection**: Bounds checking for all arithmetic operations
- **Strict Aliasing Compliance**: No undefined behavior in pointer operations
- **Volatile Qualifiers**: Proper memory visibility for shared data
- **Critical Section Protection**: Thread-safe queue operations

## Platform Support

- ✅ Linux (x86_64, ARM)
- ✅ Windows (MSVC, MinGW)
- ✅ macOS (Clang)
- ✅ FreeBSD
- ✅ Embedded Linux (ARM)
- ✅ WebAssembly

## License

MIT License - see [LICENSE](LICENSE) for details.

## References

- RFC 6229 - The RC4 Stream Cipher
- "Weaknesses in the Key Scheduling Algorithm of RC4" by Fluhrer, Mantin, Shamir

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Write tests for new functionality
4. Ensure all tests pass
5. Submit a pull request

## Disclaimer

This software is provided "as is" without warranty of any kind. Use at your own risk.
