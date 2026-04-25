#ifndef ARCFOUR_PORT_H
#define ARCFOUR_PORT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Memory allocator selection */
#ifdef ARCFOUR_STATIC_ONLY
    /* Static-only mode: no heap allocation */
    #define ARCFOUR_MALLOC  NULL
    #define ARCFOUR_FREE    NULL
#else
    /* Dynamic mode: use standard malloc/free */
    #include <stdlib.h>
    #define ARCFOUR_MALLOC  malloc
    #define ARCFOUR_FREE    free
#endif

/* Platform detection macros */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || defined(__CORTEX_M4__)
#  define ARCFOUR_CORTEX_M4 1
#elif defined(__ARM_ARCH_7A__)
#  define ARCFOUR_ARM_V7A 1
#elif defined(__ARM_ARCH_6M__) || defined(__CORTEX_M0__) || defined(__CORTEX_M0PLUS__)
#  define ARCFOUR_CORTEX_M0 1
#elif defined(__ARM_ARCH_8M_BASE__) || defined(__CORTEX_M33__)
#  define ARCFOUR_CORTEX_M33 1
#elif defined(__arm__) || defined(__TARGET_ARCH_ARM)
#  define ARCFOUR_ARM_GENERIC 1
#endif

/* Inline function support */
#if defined(__GNUC__) || defined(__clang__)
#  define ARCFOUR_INLINE static inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#  define ARCFOUR_INLINE static __forceinline
#else
#  define ARCFOUR_INLINE static inline
#endif

/* Weak symbol support */
#if defined(__GNUC__) || defined(__clang__)
#  define ARCFOUR_WEAK __attribute__((weak))
#elif defined(_MSC_VER)
#  define ARCFOUR_WEAK __declspec(selectany)
#else
#  define ARCFOUR_WEAK
#endif

/* Memory barrier */
#if defined(__GNUC__) || defined(__clang__)
#  define ARCFOUR_BARRIER() __asm__ __volatile__ ("" : : : "memory")
#elif defined(_MSC_VER)
#  define ARCFOUR_BARRIER() _ReadWriteBarrier()
#else
#  define ARCFOUR_BARRIER()
#endif

/* Critical section for Cortex-M */
#ifdef ARCFOUR_CORTEX_M4
/* Full critical section - disable all interrupts */
#  define ARCFOUR_ENTER_CRITICAL() \
    do { \
        __asm__ __volatile__ ("mrs r0, PRIMASK\n\t" \
                             "cpsid i\n\t" \
                             "push {r0}" : : : "r0", "memory"); \
    } while (0)
#  define ARCFOUR_EXIT_CRITICAL() \
    do { \
        __asm__ __volatile__ ("pop {r0}\n\t" \
                             "msr PRIMASK, r0" : : : "r0", "memory"); \
    } while (0)

/* Priority-based critical section - using BASEPRI */
#  define ARCFOUR_ENTER_CRITICAL_PRIO(prio) \
    do { \
        __asm__ __volatile__ ("mrs r0, BASEPRI\n\t" \
                             "msr BASEPRI, %0\n\t" \
                             "push {r0}" : : "r"((prio) << 4) : "r0", "memory"); \
    } while (0)
#  define ARCFOUR_EXIT_CRITICAL_PRIO() \
    do { \
        __asm__ __volatile__ ("pop {r0}\n\t" \
                             "msr BASEPRI, r0" : : : "r0", "memory"); \
    } while (0)
#elif defined(ARCFOUR_CORTEX_M33)
/* Cortex-M33 supports TrustZone, use BASEPRI */
#  define ARCFOUR_ENTER_CRITICAL() \
    do { \
        __asm__ __volatile__ ("mrs r0, PRIMASK\n\t" \
                             "cpsid i\n\t" \
                             "push {r0}" : : : "r0", "memory"); \
    } while (0)
#  define ARCFOUR_EXIT_CRITICAL() \
    do { \
        __asm__ __volatile__ ("pop {r0}\n\t" \
                             "msr PRIMASK, r0" : : : "r0", "memory"); \
    } while (0)
#  define ARCFOUR_ENTER_CRITICAL_PRIO(prio) \
    do { \
        __asm__ __volatile__ ("mrs r0, BASEPRI\n\t" \
                             "msr BASEPRI, %0\n\t" \
                             "push {r0}" : : "r"((prio) << 4) : "r0", "memory"); \
    } while (0)
#  define ARCFOUR_EXIT_CRITICAL_PRIO() \
    do { \
        __asm__ __volatile__ ("pop {r0}\n\t" \
                             "msr BASEPRI, r0" : : : "r0", "memory"); \
    } while (0)
#elif defined(ARCFOUR_CORTEX_M0)
/* Cortex-M0/M0+ only has PRIMASK (no BASEPRI) */
#  define ARCFOUR_ENTER_CRITICAL() \
    do { \
        __asm__ __volatile__ ("cpsid i" : : : "memory"); \
    } while (0)
#  define ARCFOUR_EXIT_CRITICAL() \
    do { \
        __asm__ __volatile__ ("cpsie i" : : : "memory"); \
    } while (0)
#  define ARCFOUR_ENTER_CRITICAL_PRIO(prio) ARCFOUR_ENTER_CRITICAL()
#  define ARCFOUR_EXIT_CRITICAL_PRIO() ARCFOUR_EXIT_CRITICAL()
#else
/* Generic fallback - no critical section support */
#  define ARCFOUR_ENTER_CRITICAL()
#  define ARCFOUR_EXIT_CRITICAL()
#  define ARCFOUR_ENTER_CRITICAL_PRIO(prio)
#  define ARCFOUR_EXIT_CRITICAL_PRIO()
#endif

/* ISR nesting tracking */
#if defined(ARCFOUR_CORTEX_M4) || defined(ARCFOUR_CORTEX_M33)
extern volatile uint32_t arcfour_isr_nest_count;
#  define ARCFOUR_ISR_ENTER() do { ARCFOUR_BARRIER(); arcfour_isr_nest_count++; } while(0)
#  define ARCFOUR_ISR_EXIT() do { arcfour_isr_nest_count--; ARCFOUR_BARRIER(); } while(0)
#  define ARCFOUR_IN_ISR() (arcfour_isr_nest_count > 0)
#else
#  define ARCFOUR_ISR_ENTER()
#  define ARCFOUR_ISR_EXIT()
#  define ARCFOUR_IN_ISR() (0)
#endif

/* Endian conversion utilities */
ARCFOUR_INLINE uint32_t arcfour_load32_le(const uint8_t* ptr) {
    return (uint32_t)ptr[0] |
           ((uint32_t)ptr[1] << 8) |
           ((uint32_t)ptr[2] << 16) |
           ((uint32_t)ptr[3] << 24);
}

ARCFOUR_INLINE void arcfour_store32_le(uint8_t* ptr, uint32_t value) {
    ptr[0] = (uint8_t)(value & 0xFF);
    ptr[1] = (uint8_t)((value >> 8) & 0xFF);
    ptr[2] = (uint8_t)((value >> 16) & 0xFF);
    ptr[3] = (uint8_t)((value >> 24) & 0xFF);
}

/* ARM-specific utilities */
#ifdef ARCFOUR_CORTEX_M4
ARCFOUR_INLINE void arcfour_memset32(uint32_t* dest, uint32_t value, size_t count) {
    for (size_t i = 0; i < count; i++) {
        dest[i] = value;
    }
}
#endif

/* ==================== DMA 对齐宏 ==================== */

/* Cache line size (typically 32 bytes for ARM Cortex-M) */
#define ARCFOUR_CACHE_LINE_SIZE 32

/* DMA alignment requirement (4 or 8 bytes typical) */
#define ARCFOUR_DMA_ALIGNMENT 4

/* Attribute macro: mark memory as DMA-accessible */
#if defined(__GNUC__) || defined(__clang__)
#  define ARCFOUR_DMA_BUFFER __attribute__((aligned(ARCFOUR_DMA_ALIGNMENT)))
#elif defined(_MSC_VER)
#  define ARCFOUR_DMA_BUFFER __declspec(align(ARCFOUR_DMA_ALIGNMENT))
#else
#  define ARCFOUR_DMA_BUFFER
#endif

/* Cache operation macros */
#if defined(ARCFOUR_CORTEX_M4) || defined(ARCFOUR_CORTEX_M33)
/* ARM Cortex-M cache operations using SCB registers */
#  define ARCFOUR_CACHE_INVALIDATE(addr, size) \
    do { \
        if ((addr) && (size) > 0) { \
            uint32_t __start = (uint32_t)(addr) & ~(ARCFOUR_CACHE_LINE_SIZE - 1); \
            uint32_t __end = ((uint32_t)(addr) + (size) + ARCFOUR_CACHE_LINE_SIZE - 1) & ~(ARCFOUR_CACHE_LINE_SIZE - 1); \
            for (; __start < __end; __start += ARCFOUR_CACHE_LINE_SIZE) { \
                __asm__ __volatile__ ("dcbi %0" : : "r"(__start) : "memory"); \
            } \
            __asm__ __volatile__ ("dsb" : : : "memory"); \
            __asm__ __volatile__ ("isb" : : : "memory"); \
        } \
    } while (0)

#  define ARCFOUR_CACHE_FLUSH(addr, size) \
    do { \
        if ((addr) && (size) > 0) { \
            uint32_t __start = (uint32_t)(addr) & ~(ARCFOUR_CACHE_LINE_SIZE - 1); \
            uint32_t __end = ((uint32_t)(addr) + (size) + ARCFOUR_CACHE_LINE_SIZE - 1) & ~(ARCFOUR_CACHE_LINE_SIZE - 1); \
            for (; __start < __end; __start += ARCFOUR_CACHE_LINE_SIZE) { \
                __asm__ __volatile__ ("dcbst %0" : : "r"(__start) : "memory"); \
            } \
            __asm__ __volatile__ ("dsb" : : : "memory"); \
            __asm__ __volatile__ ("isb" : : : "memory"); \
        } \
    } while (0)
#else
/* Generic fallback - no cache operations */
#  define ARCFOUR_CACHE_INVALIDATE(addr, size) (void)(addr); (void)(size)
#  define ARCFOUR_CACHE_FLUSH(addr, size) (void)(addr); (void)(size)
#endif

/* DMA double buffer structure for pipelined processing */
typedef struct {
    uint8_t* buffer0;           /* First buffer */
    uint8_t* buffer1;           /* Second buffer */
    size_t size;                /* Buffer size */
    volatile uint32_t active_buffer;  /* Current active buffer index (0 or 1) */
} arcfour_dma_double_buffer_t;

/* Power management platform abstraction */
#ifdef ARCFOUR_ENABLE_POWER_API
    /**
     * @brief Get current tick count in milliseconds
     * 
     * Used for timeout-based auto-sleep decisions.
     * Platform-specific implementation required.
     */
    uint32_t arcfour_get_tick_ms(void);
    
    /**
     * @brief Enter low-power sleep mode
     * 
     * Platform-specific implementation required.
     */
    void arcfour_enter_sleep(void);
    
    /**
     * @brief Read battery voltage in millivolts
     * 
     * Returns 0 if battery monitoring is not supported.
     * Platform-specific implementation required.
     */
    uint16_t arcfour_read_battery_mv(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ARCFOUR_PORT_H */