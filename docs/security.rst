Security Considerations
=======================

Important Warning
-----------------

**This library is for educational and research purposes only.**

RC4 algorithm has known theoretical security vulnerabilities. For production
environments, consider using modern authenticated encryption algorithms such as:

- AES-GCM
- ChaCha20-Poly1305
- AES-CCM

Security Features
-----------------

Initial Keystream Discard
~~~~~~~~~~~~~~~~~~~~~~~~~

The ARCFOUR library discards the first 50 million bytes of keystream to mitigate
the Fluhrer-Mantin-Shamir (FMS) attack:

.. code-block:: c

    // This is done automatically during initialization
    arcfour_ctx* ctx = arcfour_init(key, key_len);

Key Recommendations
-------------------

1. **Key Length**: Use keys of at least 256 bits (32 bytes)
2. **Key Randomness**: Use cryptographically secure random number generators
3. **Key Freshness**: Never reuse keys for different messages
4. **Related Keys**: Never use related keys (e.g., incrementing counters)

.. code-block:: c

    // Good: Secure random key
    uint8_t key[32];
    secure_random_bytes(key, sizeof(key));

    // Bad: Predictable key
    uint8_t bad_key[16] = "password12345678";

Side-Channel Protection
-----------------------

While the core RC4 algorithm is inherently resistant to simple side-channel
attacks, the following practices are recommended:

- Use constant-time operations where possible
- Avoid data-dependent control flow
- Consider power analysis countermeasures for embedded systems

Secure Implementation Guidelines
--------------------------------

1. **Always validate inputs** before passing to library functions
2. **Use authenticated encryption** for sensitive data (AEAD mode)
3. **Protect key material** in memory
4. **Zeroize sensitive data** after use
5. **Use secure random number generators**

.. code-block:: c

    // Zeroize key after use
    explicit_bzero(key, sizeof(key));

Known Vulnerabilities
---------------------

The RC4 algorithm has the following known vulnerabilities:

1. **FMS Attack**: Exploits biases in the initial keystream bytes
2. **RC4-drop**: Initial bytes are biased and predictable
3. **Statistical weaknesses**: Output has detectable biases

Mitigations implemented in this library:

- 50 million byte initial discard to mitigate FMS attack
- No weak key scheduling patterns
- Proper key mixing

Compliance
----------

This library is provided as-is and does not claim compliance with any specific
security standard. Users are responsible for verifying compliance with their
specific requirements.

Disclaimer
----------

This software is provided "as is" without warranty of any kind, express or
implied, including but not limited to the warranties of merchantability,
fitness for a particular purpose and noninfringement. In no event shall the
authors or copyright holders be liable for any claim, damages or other
liability, whether in an action of contract, tort or otherwise, arising from,
out of or in connection with the software or the use or other dealings in the
software.
