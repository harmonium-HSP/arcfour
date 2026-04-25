Platform Support
================

Supported Platforms
------------------

The ARCFOUR library has been tested on the following platforms:

Linux
~~~~~

- **Architectures**: x86_64, ARM, ARM64
- **Compilers**: GCC 4.8+, Clang 3.5+
- **Features**: All features supported

Windows
~~~~~~~

- **Architectures**: x86, x86_64
- **Compilers**: MSVC 2019+, MinGW-w64
- **Features**: All features supported

macOS
~~~~~

- **Architectures**: x86_64, ARM64 (Apple Silicon)
- **Compilers**: Clang (Xcode)
- **Features**: All features supported

FreeBSD
~~~~~~~

- **Architectures**: x86_64, ARM
- **Compilers**: GCC, Clang
- **Features**: All features supported

Embedded Linux
~~~~~~~~~~~~~~

- **Architectures**: ARM (Cortex-A, Cortex-M)
- **Compilers**: ARM GCC
- **Features**: Static memory API, ISR-safe API

WebAssembly
~~~~~~~~~~~

- **Compilers**: Emscripten
- **Features**: Core API only

Cross-Compilation
-----------------

ARM Cortex-M
~~~~~~~~~~~~

.. code-block:: bash

    # Using ARM GCC toolchain
    cmake -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake ..
    make

Raspberry Pi
~~~~~~~~~~~~

.. code-block:: bash

    # Cross-compile for Raspberry Pi
    cmake -DCMAKE_TOOLCHAIN_FILE=cmake/arm-linux-gnueabihf.cmake ..
    make

Build Configuration Examples
----------------------------

Linux Desktop
~~~~~~~~~~~~~

.. code-block:: bash

    mkdir build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make
    sudo make install

Windows (MSVC)
~~~~~~~~~~~~~~

.. code-block:: powershell

    mkdir build
    cd build
    cmake .. -G "Visual Studio 17 2022" -A x64
    cmake --build . --config Release

macOS
~~~~~

.. code-block:: bash

    mkdir build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make
    make install

Embedded (STM32)
~~~~~~~~~~~~~~~~

.. code-block:: bash

    # With ARM GCC toolchain
    cmake -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake \
          -DARCFOUR_STATIC_ONLY=ON \
          -DBUILD_TESTS=OFF \
          ..
    make

Platform-Specific Notes
-----------------------

Endianness
~~~~~~~~~~

The library is designed to work on both little-endian and big-endian systems.
All internal operations are byte-oriented, so no endianness issues should occur.

Memory Constraints
~~~~~~~~~~~~~~~~~~

- **Core context**: ~260 bytes (256-byte S-box + 4 bytes state)
- **Static mode**: No heap allocation required
- **Dynamic mode**: Uses malloc/free for context allocation

Thread Safety
~~~~~~~~~~~~~

- Single context: Not thread-safe
- Multiple contexts: Each context can be used independently in different threads
- ISR-safe API: Designed for interrupt context usage

Performance Notes
~~~~~~~~~~~~~~~~~

- **Initialization**: ~0.2-0.5 seconds (50M byte discard)
- **Encryption**: ~100-200 MB/s (x86_64)
- **ARM Cortex-M4**: ~20-40 MB/s

Feature Support Matrix
---------------------

.. list-table::
   :header-rows: 1

   * - Feature
     - Linux
     - Windows
     - macOS
     - Embedded
     - WASM
   * - Shared Library
     - Yes
     - Yes
     - Yes
     - No
     - No
   * - Static Library
     - Yes
     - Yes
     - Yes
     - Yes
     - Yes
   * - Static Memory API
     - Yes
     - Yes
     - Yes
     - Yes
     - Yes
   * - DMA API
     - Yes
     - Yes
     - Yes
     - Limited
     - No
   * - ISR API
     - Yes
     - No
     - No
     - Yes
     - No
   * - Power API
     - Yes
     - Yes
     - Yes
     - Yes
     - No
   * - File Utilities
     - Yes
     - Yes
     - Yes
     - No
     - No
