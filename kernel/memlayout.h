// MMIO: core local interruptor (CLINT), which contains the timer.
#define CLINT 0x2000000L
#define CLINT_MTIMECMP(hartid) (CLINT + 0x4000 + 8*(hartid)) // MTIMECMP register for each hart.
#define CLINT_MTIME (CLINT + 0xBFF8) // timer register: cycles since boot.

// MMIO: PLIC

// MMIO: UART
#define UART0 0x10000000L
