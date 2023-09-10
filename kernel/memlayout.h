#ifndef DRAM_SIZE
#define DRAM_SIZE 128
#endif

// the kernel expects there to be RAM
// for use by the kernel and user pages
// from physical address 0x80000000 to PHYSTOP.
#define KERNBASE 0x80000000L
#define PHYSTOP (KERNBASE + DRAM_SIZE*1024*1024)

// virtual address of trampoline.S
#define TRAMPOLINE (MAXVA - PGSIZE)

// MMIO: core local interruptor (CLINT), which contains the timer.
#define CLINT 0x2000000L
#define CLINT_MTIMECMP(hartid) (CLINT + 0x4000 + 8*(hartid)) // MTIMECMP register for each hart.
#define CLINT_MTIME (CLINT + 0xBFF8) // timer register: cycles since boot.

// MMIO: PLIC
#define PLIC 0x0c000000L
#define PLIC_PRIORITY (PLIC + 0x0)
#define PLIC_PENDING (PLIC + 0x1000)
#define PLIC_MENABLE(hart) (PLIC + 0x2000 + (hart)*0x100)
#define PLIC_SENABLE(hart) (PLIC + 0x2080 + (hart)*0x100)
#define PLIC_MPRIORITY(hart) (PLIC + 0x200000 + (hart)*0x2000)
#define PLIC_SPRIORITY(hart) (PLIC + 0x201000 + (hart)*0x2000)
#define PLIC_MCLAIM(hart) (PLIC + 0x200004 + (hart)*0x2000)
#define PLIC_SCLAIM(hart) (PLIC + 0x201004 + (hart)*0x2000)
#define PLIC_END 0x10000000L
#define PLIC_MMAP_SIZE (PLIC_END - PLIC)

// MMIO: UART
#define UART0 0x10000000L
#define UART0_IRQ 10

// MMIO: VIRTIO
#define VIRTIO0 0x10001000
#define VIRTIO0_IRQ 1

// Kernel stack
#define KSTACK(p) (TRAMPOLINE - (p+1)*2*PGSIZE)

// User memory layout.
// Address zero first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
//   ...
//   TRAPFRAME (p->trapframe, used by the trampoline)
//   TRAMPOLINE (the same page as in the kernel)
#define TRAPFRAME (TRAMPOLINE - PGSIZE)