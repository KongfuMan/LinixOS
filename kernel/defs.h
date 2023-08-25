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

//proc.c
void proc_mapstacks(pagetable_t);