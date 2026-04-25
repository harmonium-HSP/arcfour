# Only build unit_test and self_test in non-static-only mode (they use dynamic API)
if(NOT ARCFOUR_STATIC_ONLY)
    add_executable(unit_test test_arcfour.c)
    target_link_libraries(unit_test PRIVATE arcfour_static)
    target_compile_definitions(unit_test PRIVATE ARCFOUR_STATIC)

    add_test(NAME unit_test COMMAND unit_test)

    add_executable(self_test ../src/arcfour.c ../src/arcfour_static.c test_vectors.c)
    target_compile_definitions(self_test PRIVATE ARCFOUR_SELF_TEST)
    target_include_directories(self_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    if(WIN32)
        target_compile_definitions(self_test PRIVATE ARCFOUR_STATIC)
    endif()

    add_test(NAME self_test COMMAND self_test)
endif()

# Static memory allocation tests
add_executable(test_static test_static.c)
target_link_libraries(test_static PRIVATE arcfour_static)
target_compile_definitions(test_static PRIVATE ARCFOUR_STATIC ARCFOUR_STATIC_ONLY ARCFOUR_DISCARD_BYTES=256)

add_test(NAME test_static COMMAND test_static)

# No-heap environment tests
# Note: We compile test_no_heap.c first with -ffreestanding to avoid libc malloc dependency
add_executable(test_no_heap test_no_heap.c)
target_link_libraries(test_no_heap PRIVATE arcfour_static)
target_compile_definitions(test_no_heap PRIVATE ARCFOUR_STATIC ARCFOUR_STATIC_ONLY ARCFOUR_DISCARD_BYTES=256)

# Ensure test_no_heap's malloc/free definitions take precedence
if(UNIX)
    target_link_options(test_no_heap PRIVATE -Wl,--wrap=malloc -Wl,--wrap=free)
endif()

add_test(NAME test_no_heap COMMAND test_no_heap)

if(BUILD_AEAD_RC4)
    add_executable(aead_rc4_test test_aead_rc4.c)
    target_link_libraries(aead_rc4_test PRIVATE arcfour_static)
    target_compile_definitions(aead_rc4_test PRIVATE ARCFOUR_STATIC AEAD_RC4_STATIC)
    
    add_test(NAME aead_rc4_test COMMAND aead_rc4_test)
endif()

if(BUILD_UTILS AND BUILD_AEAD_RC4)
    add_executable(test_utils test_utils.c)
    target_link_libraries(test_utils PRIVATE arcfour_static)
    target_compile_definitions(test_utils PRIVATE ARCFOUR_STATIC AEAD_RC4_STATIC ARCFOUR_UTILS_STATIC)
    
    add_test(NAME utils_test COMMAND test_utils)
endif()

if(BUILD_POWER_API)
    add_executable(test_power test_power.c)
    target_link_libraries(test_power PRIVATE arcfour_static)
    target_compile_definitions(test_power PRIVATE ARCFOUR_STATIC ARCFOUR_ENABLE_POWER_API)
    
    if(ARCFOUR_STATIC_ONLY)
        target_compile_definitions(test_power PRIVATE ARCFOUR_STATIC_ONLY)
    endif()
    
    add_test(NAME power_test COMMAND test_power)
    
    add_executable(test_power_consumption test_power_consumption.c)
    target_link_libraries(test_power_consumption PRIVATE arcfour_static)
    target_compile_definitions(test_power_consumption PRIVATE ARCFOUR_STATIC ARCFOUR_ENABLE_POWER_API)
    
    if(ARCFOUR_STATIC_ONLY)
        target_compile_definitions(test_power_consumption PRIVATE ARCFOUR_STATIC_ONLY)
    endif()
    
    add_test(NAME power_consumption_test COMMAND test_power_consumption)
endif()

if(BUILD_ISR_API)
    add_executable(test_isr test_isr.c)
    target_link_libraries(test_isr PRIVATE arcfour_static)
    target_compile_definitions(test_isr PRIVATE ARCFOUR_STATIC ARCFOUR_ENABLE_ISR_API)
    
    add_test(NAME isr_test COMMAND test_isr)
endif()

if(BUILD_DMA_API)
    add_executable(test_dma test_dma.c)
    target_link_libraries(test_dma PRIVATE arcfour_static)
    target_compile_definitions(test_dma PRIVATE ARCFOUR_STATIC ARCFOUR_ENABLE_DMA_API)
    
    if(ARCFOUR_STATIC_ONLY)
        target_compile_definitions(test_dma PRIVATE ARCFOUR_STATIC_ONLY)
    endif()
    
    add_test(NAME dma_test COMMAND test_dma)
endif()