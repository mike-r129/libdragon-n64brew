#include <malloc.h>

void test_dma_read_misalign(TestContext *ctx) {
	uint32_t rom = dfs_rom_addr("counter.dat");
	uint8_t rom_copy[4096] __attribute__((aligned(16)));
	uint8_t *ram = memalign(0x1000, 8192);
	DEFER(free(ram));

	data_cache_hit_writeback_invalidate(rom_copy, sizeof(rom_copy));
	dma_read(rom_copy, rom, 4096);

	memset(ram, 0xAA, 8192);
	data_cache_hit_writeback_invalidate(ram, 8192);

	static const uint8_t expAA[16] = { 0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA, };

	void run(int ram_offset, int rom_offset, int length) {
		dma_read(ram+ram_offset, rom+rom_offset, length);

		ASSERT_EQUAL_MEM(ram+ram_offset-16, expAA, 16, "invalid prefix [%d/%d/%d]", ram_offset, rom_offset, length);
		ASSERT_EQUAL_MEM(ram+ram_offset, rom_copy+rom_offset, length, "invalid data [0x%x/%d/%d]", ram_offset, rom_offset, length);
		ASSERT_EQUAL_MEM(ram+ram_offset+length, expAA, 16, "invalid suffix [0x%x/%d/%d]", ram_offset, rom_offset, length);

		memset(ram+ram_offset, 0xAA, length);
		data_cache_hit_writeback_invalidate(ram+ram_offset, length+1);
	}

	for (int i=0x7e0; i<0x800; i++) {
		for (int j=1;j<224;j++) {
			run(i, i&1, j);
			if (ctx->result == TEST_FAILED)
				return;
		}
	}
}

void test_dma_enqueue_io_busy(TestContext *ctx) {
	// A CPU access to the PI bus leaves PI_STATUS IO-busy for a moment and no
	// interrupt follows. A transfer enqueued in that window must still be
	// started: nothing else would start it.
	uint32_t rom = dfs_rom_addr("counter.dat");
	uint8_t *buf = malloc_uncached(64);
	DEFER(free_uncached(buf));

	int stuck = 0, first = -1;
	for (int k = 0; k < 200; k++) {
		dma_wait();
		*(volatile uint32_t *)0xB0000000 = k;  // CPU write to the cartridge bus (ROM ignores it)
		uint64_t t = dma_read_async(buf, rom, 64);

		uint32_t t0 = TICKS_READ();
		while (!dma_finished(t) && TICKS_SINCE(t0) < TICKS_FROM_MS(2)) {}
		if (!dma_finished(t)) {
			if (first < 0) first = k;
			stuck++;
			// Recover, so that the rest of the test ROM can run: an enqueue
			// on an idle PI starts the queue.
			dma_read_async(buf + 32, rom, 32);
			dma_wait_finished(t);
		}
	}
	dma_wait();
	ASSERT_EQUAL_SIGNED(stuck, 0, "transfers never started: %d of 200 (first: round %d)", stuck, first);
}
