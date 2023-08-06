// apis that public to kernel.


// console.c
void consoleinit();

// uart.c
void uartinit();
void uart_putc(char c);
char uart_getc();
