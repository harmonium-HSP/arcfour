# ARCFOUR Python Bindings

Python CFFI bindings for the ARCFOUR cryptographic library.

## Installation

```bash
pip install arcfour-crypto
```

## Usage

```python
from arcfour import ARCFOUR

# Create cipher instance with key
key = b'secret_key_32_bytes_long!!'
cipher = ARCFOUR(key)

# Encrypt data
plaintext = b'Hello, World!'
ciphertext = cipher.encrypt(plaintext)

# Decrypt data  
decrypted = cipher.decrypt(ciphertext)
assert decrypted == plaintext
```

## API

### ARCFOUR Class

```python
class ARCFOUR:
    def __init__(self, key: bytes):
        """Initialize with a key (16-32 bytes recommended)"""
    
    def encrypt(self, data: bytes) -> bytes:
        """Encrypt data in-place"""
    
    def decrypt(self, data: bytes) -> bytes:
        """Decrypt data in-place"""
```

## Development

```bash
# Install development dependencies
pip install cffi pytest

# Build the bindings
python setup.py build_ext --inplace

# Run tests
pytest test_arcfour.py
```

## License

MIT License
