#include <malloc.h>

void test_sys_hwmemset(TestContext *ctx) {
    const int BUFFER_SIZE = 4096;

    // Allocate a 4K buffer, aligned to 2K (RDRAM row)
    uint8_t *buf = memalign(2048, BUFFER_SIZE);
    DEFER(free(buf));

    // Set different lengths, and check the result
    static const int lengths[] = { 1, 2, 3, 7, 8, 9, 16, 17, 31, 32, 33, 63, 64,
                                   65, 127, 128, 129, 255, 256, 257, 511, 512, 513, 1023, 1024 };
    static const int offsets[] = { 0, 1, 2, 3, 4, 5, 6, 7, 126, 127, 128, 2039, 2040, 2044, 2045, 2046, 2047 };

    for (int i=0; i<sizeof(lengths)/sizeof(lengths[0]); i++) {
        int len = lengths[i];

        for (int j=0; j<sizeof(offsets)/sizeof(offsets[0]); j++) {
            int off = offsets[j];

            memset(buf, 0xAA, BUFFER_SIZE);
            sys_hw_memset(buf+off, 0x44, len);

            // Check the result
            for (int i=0; i<BUFFER_SIZE; i++) {
                uint8_t expected = (i >= off && i < off+len) ? 0x44 : 0xAA;

                ASSERT_EQUAL_HEX(buf[i], expected, 
                    "sys_hw_memset len=%d off=%d failed at byte %d", len, off, i);
            }
        }
    }
}

void test_sys_hwmemset_uncached(TestContext *ctx) {
    const int BUFFER_SIZE = 4096;

    // Allocate a 4K buffer, aligned to 2K (RDRAM row)
    uint8_t *buf = malloc_uncached_aligned(2048, BUFFER_SIZE);
    DEFER(free_uncached(buf));

    // Set different lengths, and check the result
    static const int lengths[] = { 3, 7, 8, 9, 16, 64, 65, 127, 128, 511, 512, 1024 };
    static const int offsets[] = { 0, 4, 7, 127, 128, 2046, 2047 };

    for (int i=0; i<sizeof(lengths)/sizeof(lengths[0]); i++) {
        int len = lengths[i];

        for (int j=0; j<sizeof(offsets)/sizeof(offsets[0]); j++) {
            int off = offsets[j];

            memset(buf, 0xAA, BUFFER_SIZE);
            sys_hw_memset(buf+off, 0x44, len);

            // Check the result
            for (int i=0; i<BUFFER_SIZE; i++) {
                uint8_t expected = (i >= off && i < off+len) ? 0x44 : 0xAA;

                ASSERT_EQUAL_HEX(buf[i], expected, 
                    "sys_hw_memset uncached len=%d off=%d failed at byte %d", len, off, i);
            }
        }
    }
}

void test_sys_hwmemset32(TestContext *ctx) {
    const int BUFFER_SIZE = 1024;

    // Values with bit 31 set are passed sign-extended in a 64-bit register
    // and must still replicate as 32 bits
    static const uint32_t values[] = { 0x12345678, 0x7EADBEEF, 0x80000001, 0xDEADBEEF, 0xFFFFFFFF, 0 };
    static const int lengths[] = { 4, 8, 12, 16, 20, 64, 68, 132, 256 };
    static const int offsets[] = { 0, 4, 8, 12, 124, 128 };

    uint32_t *buf = memalign(2048, BUFFER_SIZE);
    DEFER(free(buf));

    for (int v=0; v<sizeof(values)/sizeof(values[0]); v++) {
        for (int i=0; i<sizeof(lengths)/sizeof(lengths[0]); i++) {
            for (int j=0; j<sizeof(offsets)/sizeof(offsets[0]); j++) {
                int len = lengths[i], off = offsets[j];

                memset(buf, 0xAA, BUFFER_SIZE);
                sys_hw_memset32((uint8_t*)buf+off, values[v], len);

                for (int w=0; w<BUFFER_SIZE/4; w++) {
                    uint32_t expected = (w*4 >= off && w*4 < off+len) ? values[v] : 0xAAAAAAAA;

                    ASSERT_EQUAL_HEX(buf[w], expected,
                        "sys_hw_memset32 value=%08lx len=%d off=%d failed at word %d",
                        values[v], len, off, w);
                }
            }
        }
    }
}
