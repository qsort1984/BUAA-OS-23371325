#include <lib.h>

#define SADDR1 0x50000000
#define EADDR1 0x55000000

static int do_alloc(u_int start_addr, u_int end_addr) {
	u_int va = 0;
	for (va = start_addr; va < end_addr; va += PAGE_SIZE) {
		try(syscall_mem_alloc(0, (void *)va, PTE_D));

		for (int i = 0; i < PAGE_SIZE; ++i) {
			*(char *)(va + i) = i % 26 + 'a';
		}
		if (va % 0x1000000 == 0) {
			debugf("alloc for va: 0x%x ok!\n", va);
		}
	}
	debugf("alloc for va: 0x%x ok!\n", va);

	for (va = start_addr; va < end_addr; va += PAGE_SIZE) {
		for (int i = 0; i < PAGE_SIZE; ++i) {
			if (*(char *)(va + i) != (i % 26) + 'a') {
				debugf("unmap check error: i: %d ,va : %08x read %d, but answer is "
				       "%d\n",
				       i, va + i, *(char *)(va + i), (i % 26) + 'a');
				return -1;
			}
		}
		try(syscall_mem_unmap(0, (void *)va));
		if (va % 0x1000000 == 0) {
			debugf("unmap for va: 0x%x ok!\n", va);
		}
	}
	debugf("unmap for va: 0x%x ok!\n", va);
	return 0;
}

int main(int argc, char **argv) {
	debugf("swap test begin!\n");
	if (do_alloc(SADDR1, EADDR1) != 0) {
		debugf("swap test failed\n");
		return -1;
	}
	debugf("swap test succeed!\n");
	return 0;
}
