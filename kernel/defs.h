/* below are APIs public to kernel. */

// console.c
void consoleinit(void);
void consoleintr(char);
void consputc(char);

// uart.c
void uartinit(void);
void uart_putc(char c);
char uart_getc(void);

// kalloc.c
void kinit(void);
void* kalloc(void);
void kfree(void*);

// string.c
void* memset(void*, int, uint);

// printf.c
void printf(char*, ...);
void panic(char*);

// vm.c
void kvminit(void);
void kvmmap(pagetable_t, uint64, uint64, uint64, int);
void kvminithart(void);
int mappages(pagetable_t, uint64, uint64, uint64, int);
pagetable_t uvmcreate(void);

//proc.c
void proc_mapstacks(pagetable_t);
void procinit(void);
int cpuid(void);
void userinit(void);
void scheduler(void);

//trap.c
void trapinit(void);
void trapinithart(void);
void usertrap(void);
void usertrapret(void);

//plic.c
void plicinit(void);
void plicinithart(void);

//swtch.S
void swtch(struct context *from, struct context *to);