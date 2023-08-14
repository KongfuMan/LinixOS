#include "types.h"

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
