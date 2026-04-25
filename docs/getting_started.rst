Getting Started
===============

This guide will help you get started with the ARCFOUR library.

Prerequisites
-------------

- CMake 3.16+
- GCC, Clang, or MSVC
- Python 3.6+ (for Python bindings)

Building from Source
--------------------

.. code-block:: bash

    # Clone the repository
    git clone https://github.com/harmonium-HSP/arcfour.git
    cd arcfour

    # Create build directory
    mkdir build && cd build

    # Configure with CMake
    cmake ..

    # Build
    make

    # Run tests
    make test

Build Options
-------------

The following CMake options are available:

.. list-table::
   :header-rows: 1

   * - Option
     - Description
     - Default
   * - BUILD_SHARED_LIBS
     - Build shared library
     - ON
   * - BUILD_STATIC_LIBS
     - Build static library
     - ON
   * - ARCFOUR_STATIC_ONLY
     - Static memory allocation only
     - OFF
   * - BUILD_DMA
     - Enable DMA-optimized code
     - OFF
   * - BUILD_ISR_API
     - Enable interrupt-safe API
     - OFF
   * - BUILD_POWER_API
     - Enable power-aware API
     - OFF
   * - BUILD_TESTS
     - Build test suite
     - ON
   * - BUILD_EXAMPLES
     - Build example programs
     - ON

Cross-Compilation
-----------------

For ARM Cortex-M targets:

.. code-block:: bash

    cmake -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake ..
    make

Installation
------------

.. code-block:: bash

    # Install to system
    make install

    # Uninstall
    make uninstall
