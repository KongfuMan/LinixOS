//
// formatted console output -- printf, panic.
//

#include <stdarg.h>

#include "param.h"
#include "types.h"
// #include "spinlock.h"
// #include "sleeplock.h"
// #include "fs.h"
// #include "file.h"
#include "memlayout.h"
#include "riscv.h"
#include "proc.h"
#include "defs.h"

volatile int panicked = 0;

// lock to avoid interleaving concurrent printf's.
// static struct {
//   struct spinlock lock;
//   int locking;
// } pr;

static char digits[] = "0123456789abcdef";

static void printint(int xx, int base, int sign)
{
    char buf[16];
    int i;
    uint x;

    if(sign && (sign = xx < 0))
        x = -xx;
    else
        x = xx;

    i = 0;
    do {
        buf[i++] = digits[x % base];
    } while((x /= base) != 0);

    if(sign)
        buf[i++] = '-';

    while(--i >= 0)
        uart_putc(buf[i]);
}

// static void
// printptr(uint64 x)
// {
//   int i;
//   consputc('0');
//   consputc('x');
//   for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
//     consputc(digits[x >> (sizeof(uint64) * 8 - 4)]);
// }

// Print to the console. only understands %d, %x, %p, %s.
void printf(char *fmt, ...)
{
    va_list ap;
    int i, c;
    char *s;
    if (fmt == 0)
        panic("null fmt");

    // TODO: lock to prevent interleaving printf for multi hart system

    va_start(ap, fmt);
    for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
        if (c!='%'){
            uart_putc(c);
            continue;
        }

        c = fmt[++i] & 0xff;
        if(c == 0)
            break;
        switch(c){
            case 'd':
                printint(va_arg(ap, int), 10, 1);
                break;
            case 'x':
                printint(va_arg(ap, int), 16, 1);
                break;
            // case 'p':
            //     printptr(va_arg(ap, uint64));
            //     break;
            case 's':
                if((s = va_arg(ap, char*)) == 0)
                    s = "(null)";
                for(; *s; s++)
                    uart_putc(*s);
                break;
            case '%':
                uart_putc('%');
                break;
            default:
                // Print unknown % sequence to draw attention.
                uart_putc('%');
                uart_putc(c);
            break;
        }
    }
    va_end(ap);
}

void panic(char *s)
{
    printf("panic: ");
    printf(s);
    printf("\n");
    panicked = 1; // freeze uart output from other CPUs
    for(;;)
        ;
}

// void
// printfinit(void)
// {
//   initlock(&pr.lock, "pr");
//   pr.locking = 1;
// }