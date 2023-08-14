
// the kernel expects there to be RAM
// for use by the kernel and user pages
// from physical address 0x80000000 to PHYSTOP.
#define KERNBASE 0x80000000L
#define PHYSTOP (KERNBASE + 128*1024*1024) // 128Mb RAM

// MMIO: core local interruptor (CLINT), which contains the timer.
#define CLINT 0x2000000L
#define CLINT_MTIMECMP(hartid) (CLINT + 0x4000 + 8*(hartid)) // MTIMECMP register for each hart.
#define CLINT_MTIME (CLINT + 0xBFF8) // timer register: cycles since boot.

// MMIO: PLIC

// MMIO: UART
#define UART0 0x10000000L
