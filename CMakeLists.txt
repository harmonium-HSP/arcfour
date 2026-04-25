cmake_minimum_required(VERSION 3.16)
project(arcfour
        VERSION 1.0.0
        DESCRIPTION "Improved RC4 stream cipher library with AEAD mode"
        LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Windows only builds static libraries to avoid DLL dependency issues
if(WIN32)
    option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
else()
    option(BUILD_SHARED_LIBS "Build shared libraries" ON)
endif()
option(BUILD_TESTING "Build tests" ON)
option(BUILD_EXAMPLE "Build example" ON)
option(BUILD_AEAD_RC4 "Build RC4 + Poly1305 AEAD mode" ON)
option(BUILD_UTILS "Build file encryption utilities" ON)
option(BUILD_BENCHMARK "Build performance benchmark" OFF)
option(BUILD_WASM "Build WebAssembly module" OFF)
option(BUILD_ARM_OPT "Build ARM Cortex-M optimized version" OFF)
option(BUILD_POWER_API "Build power-aware API extensions" OFF)
option(BUILD_ISR_API "Build interrupt-safe API extensions" OFF)
option(BUILD_DMA_API "Build DMA-friendly buffer alignment extensions" OFF)
option(BUILD_FUZZER "Build fuzz test target" OFF)
option(ARCFOUR_STATIC_ONLY "Build static-only version without malloc/free" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler-specific flags
if(MSVC)
    # MSVC compiler options
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /W4 /Od")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /W4 /O2")
    # Disable specific warnings
    add_compile_options(/wd4996)  # Disable deprecation warnings
else()
    # GCC/Clang compiler options
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Wextra -g")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wall -Wextra -O2")
endif()

include(CheckSymbolExists)
check_symbol_exists(explicit_bzero "string.h" HAVE_EXPLICIT_BZERO)
check_symbol_exists(memset_s "string.h" HAVE_MEMSET_S)

configure_file(cmake/config.h.in config.h)

include_directories(include)
include_directories(${PROJECT_BINARY_DIR})

set(arcfour_SOURCES
    src/arcfour_static.c
)

if(NOT ARCFOUR_STATIC_ONLY)
    list(APPEND arcfour_SOURCES src/arcfour.c)
endif()

if(BUILD_AEAD_RC4)
    list(APPEND arcfour_SOURCES src/aead_rc4.c)
endif()

if(BUILD_UTILS)
    list(APPEND arcfour_SOURCES src/arcfour_utils.c)
endif()

if(BUILD_ARM_OPT)
    list(APPEND arcfour_SOURCES 
        src/arm/arcfour_arm.c
        src/common/arcfour_generic.c
    )
endif()

if(BUILD_POWER_API)
    list(APPEND arcfour_SOURCES src/arcfour_power.c)
    if(BUILD_ARM_OPT)
        list(APPEND arcfour_SOURCES src/arm/arcfour_power_arm.c)
    endif()
endif()

if(BUILD_ISR_API)
    list(APPEND arcfour_SOURCES src/arcfour_isr.c)
    if(BUILD_ARM_OPT)
        list(APPEND arcfour_SOURCES src/arm/arcfour_isr_arm.c)
    endif()
endif()

if(BUILD_DMA_API)
    list(APPEND arcfour_SOURCES src/arcfour_dma.c)
    if(BUILD_ARM_OPT)
        list(APPEND arcfour_SOURCES src/arm/arcfour_dma_arm.c)
    endif()
endif()

if(NOT ARCFOUR_STATIC_ONLY)
    add_library(arcfour_shared SHARED ${arcfour_SOURCES})
    set_target_properties(arcfour_shared PROPERTIES
        OUTPUT_NAME arcfour
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
endif()

add_library(arcfour_static STATIC ${arcfour_SOURCES})
set_target_properties(arcfour_static PROPERTIES
    OUTPUT_NAME arcfour
)

if(WIN32)
    if(NOT ARCFOUR_STATIC_ONLY)
        target_compile_definitions(arcfour_shared PRIVATE ARCFOUR_EXPORTS)
        if(BUILD_AEAD_RC4)
            target_compile_definitions(arcfour_shared PRIVATE AEAD_RC4_EXPORTS)
        endif()
        if(BUILD_UTILS)
            target_compile_definitions(arcfour_shared PRIVATE ARCFOUR_UTILS_EXPORTS)
        endif()
    endif()
    target_compile_definitions(arcfour_static PRIVATE ARCFOUR_STATIC)
    if(BUILD_AEAD_RC4)
        target_compile_definitions(arcfour_static PRIVATE AEAD_RC4_STATIC)
    endif()
    if(BUILD_UTILS)
        target_compile_definitions(arcfour_static PRIVATE ARCFOUR_UTILS_STATIC)
    endif()
elseif(NOT ARCFOUR_STATIC_ONLY)
    set_target_properties(arcfour_shared PROPERTIES
        C_VISIBILITY_PRESET hidden
    )
    target_compile_options(arcfour_shared PRIVATE -fvisibility=hidden)
endif()

# Add ARCFOUR_STATIC_ONLY definition for static-only builds
if(ARCFOUR_STATIC_ONLY)
    target_compile_definitions(arcfour_static PRIVATE ARCFOUR_STATIC_ONLY)
endif()

if(NOT ARCFOUR_STATIC_ONLY)
    install(TARGETS arcfour_shared arcfour_static
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
            RUNTIME DESTINATION bin)
else()
    install(TARGETS arcfour_static
            ARCHIVE DESTINATION lib)
endif()

install(FILES include/arcfour.h DESTINATION include)

if(BUILD_AEAD_RC4)
    install(FILES include/aead_rc4.h DESTINATION include)
endif()

if(BUILD_UTILS)
    install(FILES include/arcfour_utils.h DESTINATION include)
endif()

if(BUILD_ARM_OPT)
    list(APPEND arcfour_SOURCES 
        src/arm/arcfour_arm.c
        src/common/arcfour_generic.c
    )
    install(FILES include/arcfour_port.h include/arcfour_arm.h DESTINATION include)
    if(NOT ARCFOUR_STATIC_ONLY)
        target_compile_definitions(arcfour_shared PRIVATE ARCFOUR_USE_ARM_OPT)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm" OR CMAKE_C_COMPILER_TARGET MATCHES "arm")
            target_compile_options(arcfour_shared PRIVATE -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard)
        endif()
    endif()
    target_compile_definitions(arcfour_static PRIVATE ARCFOUR_USE_ARM_OPT)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm" OR CMAKE_C_COMPILER_TARGET MATCHES "arm")
        target_compile_options(arcfour_static PRIVATE -mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard)
    endif()
endif()

if(BUILD_POWER_API)
    install(FILES include/arcfour_power.h DESTINATION include)
    install(FILES include/arcfour_port.h DESTINATION include)
endif()

if(BUILD_ISR_API)
    target_compile_definitions(arcfour_static PRIVATE ARCFOUR_ENABLE_ISR_API)
    if(NOT ARCFOUR_STATIC_ONLY)
        target_compile_definitions(arcfour_shared PRIVATE ARCFOUR_ENABLE_ISR_API)
    endif()
    install(FILES include/arcfour_isr.h DESTINATION include)
    install(FILES include/arcfour_port.h DESTINATION include)
endif()

if(BUILD_DMA_API)
    target_compile_definitions(arcfour_static PRIVATE ARCFOUR_ENABLE_DMA_API)
    if(NOT ARCFOUR_STATIC_ONLY)
        target_compile_definitions(arcfour_shared PRIVATE ARCFOUR_ENABLE_DMA_API)
    endif()
    install(FILES include/arcfour_dma.h DESTINATION include)
    install(FILES include/arcfour_port.h DESTINATION include)
endif()

if(BUILD_POWER_API)
    install(FILES include/arcfour_port.h DESTINATION include)
    if(NOT ARCFOUR_STATIC_ONLY)
        target_compile_definitions(arcfour_shared PRIVATE ARCFOUR_ENABLE_POWER_API)
    endif()
    target_compile_definitions(arcfour_static PRIVATE ARCFOUR_ENABLE_POWER_API)
endif()

configure_file(cmake/arcfour.pc.in arcfour.pc @ONLY)
install(FILES ${PROJECT_BINARY_DIR}/arcfour.pc DESTINATION lib/pkgconfig)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()

if(BUILD_EXAMPLE AND NOT ARCFOUR_STATIC_ONLY)
    add_subdirectory(example)
endif()

if(BUILD_BENCHMARK AND BUILD_AEAD_RC4)
    add_executable(bench_aead benchmark/bench_aead.c)
    target_link_libraries(bench_aead arcfour_static)
    target_include_directories(bench_aead PRIVATE include)
    target_compile_definitions(bench_aead PRIVATE ARCFOUR_STATIC AEAD_RC4_STATIC)
endif()

if(BUILD_WASM AND BUILD_AEAD_RC4)
    find_program(EMCC emcc)
    if(EMCC)
        set(WASM_SOURCES
            src/arcfour.c
            src/poly1305.c
            src/aead_rc4.c
            bindings/wasm/arcfour_wasm.c
        )
        
        set(WASM_OUTPUT_DIR ${PROJECT_BINARY_DIR}/wasm)
        file(MAKE_DIRECTORY ${WASM_OUTPUT_DIR})
        
        set(WASM_CFLAGS
            -O3
            -s WASM=1
            "-s EXPORTED_FUNCTIONS=['_aead_rc4_encrypt_wasm', '_aead_rc4_decrypt_wasm', '_arcfour_encrypt_wasm', '_free_wasm', '_generate_nonce_wasm']"
            "-s EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap', 'malloc', 'free']"
            -s ALLOW_MEMORY_GROWTH=1
            -I${PROJECT_SOURCE_DIR}/include
        )
        
        add_custom_command(
            OUTPUT ${WASM_OUTPUT_DIR}/arcfour.js ${WASM_OUTPUT_DIR}/arcfour.wasm
            COMMAND ${EMCC}
                ${WASM_CFLAGS}
                ${WASM_SOURCES}
                -o ${WASM_OUTPUT_DIR}/arcfour.js
                --pre-js ${PROJECT_SOURCE_DIR}/bindings/wasm/preamble.js
            MAIN_DEPENDENCY ${WASM_SOURCES}
            COMMENT "Building WebAssembly module..."
        )
        
        add_custom_target(wasm ALL
            DEPENDS ${WASM_OUTPUT_DIR}/arcfour.js ${WASM_OUTPUT_DIR}/arcfour.wasm
        )
        
        file(COPY ${PROJECT_SOURCE_DIR}/bindings/wasm/index.html
             DESTINATION ${WASM_OUTPUT_DIR})
        
        message(STATUS "WASM build enabled")
    else()
        message(WARNING "emcc not found. WebAssembly build disabled.")
    endif()
endif()

if(BUILD_FUZZER)
    find_package(LLVM REQUIRED CONFIG)
    find_program(CLANGXX clang++)
    
    if(CLANGXX)
        set(CMAKE_C_COMPILER clang)
        set(CMAKE_CXX_COMPILER clang++)
        
        # Fuzz target
        add_executable(fuzz_arcfour fuzz/fuzz_arcfour.cc)
        target_link_libraries(fuzz_arcfour PRIVATE arcfour_static)
        target_include_directories(fuzz_arcfour PRIVATE include src)
        
        # Enable sanitizers
        target_compile_options(fuzz_arcfour PRIVATE 
            -fsanitize=fuzzer,address,undefined
            -fno-omit-frame-pointer
            -fno-optimize-sibling-calls
            -O1
        )
        target_link_options(fuzz_arcfour PRIVATE 
            -fsanitize=fuzzer,address,undefined
        )
        
        # Copy dictionary file
        configure_file(fuzz/fuzz_arcfour.dict fuzz_arcfour.dict COPYONLY)
        
        message(STATUS "Fuzzer build enabled")
    else()
        message(WARNING "clang++ not found. Fuzzer build disabled.")
    endif()
endif()

message(STATUS "ARCFOUR build configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Shared libs: ${BUILD_SHARED_LIBS}")
message(STATUS "  AEAD RC4: ${BUILD_AEAD_RC4}")
message(STATUS "  Utils: ${BUILD_UTILS}")
message(STATUS "  ARM opt: ${BUILD_ARM_OPT}")
message(STATUS "  Benchmark: ${BUILD_BENCHMARK}")
message(STATUS "  WASM: ${BUILD_WASM}")
message(STATUS "  Tests: ${BUILD_TESTING}")
message(STATUS "  Example: ${BUILD_EXAMPLE}")
message(STATUS "  Fuzzer: ${BUILD_FUZZER}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")