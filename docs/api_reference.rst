API Reference
=============

Core API
--------

.. c:function:: arcfour_ctx* arcfour_init(const uint8_t* key, size_t key_len)

    Initialize ARCFOUR context with a key.

    :param key: Key data (must be non-NULL)
    :param key_len: Key length in bytes (must be > 0)
    :return: Initialized context pointer, or NULL on failure

.. c:function:: void arcfour_uninit(arcfour_ctx* ctx)

    Release ARCFOUR context.

    :param ctx: Context to release (can be NULL)

.. c:function:: void arcfour_encrypt(arcfour_ctx* ctx, const uint8_t* plaintext, uint8_t* ciphertext, size_t len)

    Encrypt data using the ARCFOUR stream cipher.

    :param ctx: ARCFOUR context
    :param plaintext: Input plaintext data
    :param ciphertext: Output ciphertext
    :param len: Length of data in bytes

.. c:function:: void arcfour_decrypt(arcfour_ctx* ctx, const uint8_t* ciphertext, uint8_t* plaintext, size_t len)

    Decrypt data using the ARCFOUR stream cipher.

    :param ctx: ARCFOUR context
    :param ciphertext: Input ciphertext data
    :param plaintext: Output plaintext
    :param len: Length of data in bytes

Static Memory API
-----------------

.. c:function:: void arcfour_init_static(arcfour_ctx_t* ctx, const uint8_t* key, size_t key_len)

    Initialize ARCFOUR context using static memory.

    :param ctx: Pointer to context structure
    :param key: Key data
    :param key_len: Key length

.. c:function:: void arcfour_encrypt_static(arcfour_ctx_t* ctx, const uint8_t* plaintext, uint8_t* ciphertext, size_t len)

    Encrypt data using static context.

    :param ctx: ARCFOUR static context
    :param plaintext: Input plaintext
    :param ciphertext: Output ciphertext
    :param len: Data length

Power-Aware API
---------------

.. c:type:: arcfour_power_hooks_t

    Structure containing power management callbacks.

    .. c:member:: int (*before_operation)(void)

        Called before encryption operation.

    .. c:member:: void (*after_operation)(void)

        Called after encryption operation.

    .. c:member:: void (*on_low_battery)(void)

        Called when low battery is detected.

    .. c:member:: uint32_t timeout_ms

        Timeout in milliseconds.

.. c:function:: arcfour_ctx* arcfour_init_power_aware(const uint8_t* key, size_t key_len, const arcfour_power_hooks_t* hooks)

    Initialize power-aware ARCFOUR context.

    :param key: Key data
    :param key_len: Key length
    :param hooks: Power management hooks
    :return: Context pointer or NULL

.. c:function:: int arcfour_encrypt_power_aware(arcfour_ctx* ctx, const uint8_t* plaintext, uint8_t* ciphertext, size_t len, const arcfour_power_hooks_t* hooks)

    Encrypt with power management.

    :param ctx: ARCFOUR context
    :param plaintext: Input data
    :param ciphertext: Output data
    :param len: Data length
    :param hooks: Power management hooks
    :return: 0 on success, negative on failure

Utility Functions
-----------------

.. c:function:: void arcfour_skip(arcfour_ctx* ctx, size_t n_bytes)

    Skip n_bytes of keystream.

    :param ctx: ARCFOUR context
    :param n_bytes: Number of bytes to skip

.. c:function:: void arcfour_copy(arcfour_ctx* dest, const arcfour_ctx* src)

    Copy context state.

    :param dest: Destination context
    :param src: Source context

.. c:function:: int arcfour_key_setup(arcfour_ctx** ctx, const uint8_t* password, size_t pass_len, const uint8_t* salt, size_t salt_len, unsigned int iterations)

    Derive key from password using PBKDF2.

    :param ctx: Output context pointer
    :param password: User password
    :param pass_len: Password length
    :param salt: Salt value
    :param salt_len: Salt length (minimum 8 bytes)
    :param iterations: PBKDF2 iterations (minimum 10000)
    :return: 0 on success, -1 on failure

.. c:function:: int arcfour_self_test(void)

    Run built-in self-test.

    :return: 0 if all tests pass, non-zero for failures
