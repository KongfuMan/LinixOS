//
// formatted console output -- printf, panic.
//

#include <stdarg.h>

#include "types.h"
#include "param.h"
// #include "spinlock.h"
// #include "sleeplock.h"
// #include "fs.h"
// #include "file.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

volatile int panicked = 0;

// lock to avoid interleaving concurrent printf's.
// static struct {
//   struct spinlock lock;
//   int locking;
// } pr;

// static char digits[] = "0123456789abcdef";

// static void
// printint(int xx, int base, int sign)
// {
//   char buf[16];
//   int i;
//   uint x;

//   if(sign && (sign = xx < 0))
//     x = -xx;
//   else
//     x = xx;

//   i = 0;
//   do {
//     buf[i++] = digits[x % base];
//   } while((x /= base) != 0);

//   if(sign)
//     buf[i++] = '-';

//   while(--i >= 0)
//     consputc(buf[i]);
// }

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
    int i, c;
    if (fmt == 0)
        panic("null fmt");

    for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
        uart_putc(c);
    }
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
