#include "defs.h"

void test_uart_putc(){
    char c = '&';
    uart_putc(c);
    uart_putc(c);
    uart_putc(c);
}

void main(){
    consoleinit();
    test_uart_putc();
    while(1){
    }
}