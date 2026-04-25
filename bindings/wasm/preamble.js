if (typeof Module === 'undefined') Module = {};

Module.onRuntimeInitialized = Module.onRuntimeInitialized || function() {};

var _aead_rc4_encrypt_wasm = function(key, key_len, nonce, nonce_len, aad, aad_len, plaintext, plaintext_len, output_len) {
    return Module.ccall(
        'aead_rc4_encrypt_wasm',
        'number',
        ['number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number'],
        [key, key_len, nonce, nonce_len, aad, aad_len, plaintext, plaintext_len, output_len]
    );
};

var _aead_rc4_decrypt_wasm = function(key, key_len, nonce, nonce_len, aad, aad_len, ciphertext, ciphertext_len, tag, output_len) {
    return Module.ccall(
        'aead_rc4_decrypt_wasm',
        'number',
        ['number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number'],
        [key, key_len, nonce, nonce_len, aad, aad_len, ciphertext, ciphertext_len, tag, output_len]
    );
};

var _arcfour_encrypt_wasm = function(key, key_len, plaintext, plaintext_len, output_len) {
    return Module.ccall(
        'arcfour_encrypt_wasm',
        'number',
        ['number', 'number', 'number', 'number', 'number'],
        [key, key_len, plaintext, plaintext_len, output_len]
    );
};

var _free_wasm = function(ptr) {
    Module.ccall('free_wasm', 'void', ['number'], [ptr]);
};

var _generate_nonce_wasm = function(output_len) {
    return Module.ccall(
        'generate_nonce_wasm',
        'number',
        ['number'],
        [output_len]
    );
};

Module._aead_rc4_encrypt_wasm = function(key, key_len, nonce, nonce_len, aad, aad_len, plaintext, plaintext_len, output_len) {
    return _aead_rc4_encrypt_wasm(key, key_len, nonce, nonce_len, aad, aad_len, plaintext, plaintext_len, output_len);
};

Module._aead_rc4_decrypt_wasm = function(key, key_len, nonce, nonce_len, aad, aad_len, ciphertext, ciphertext_len, tag, output_len) {
    return _aead_rc4_decrypt_wasm(key, key_len, nonce, nonce_len, aad, aad_len, ciphertext, ciphertext_len, tag, output_len);
};

Module._arcfour_encrypt_wasm = function(key, key_len, plaintext, plaintext_len, output_len) {
    return _arcfour_encrypt_wasm(key, key_len, plaintext, plaintext_len, output_len);
};

Module._free_wasm = function(ptr) {
    _free_wasm(ptr);
};

Module._generate_nonce_wasm = function(output_len) {
    return _generate_nonce_wasm(output_len);
};