#include "param.h"
#include "types.h"
#include "riscv.h"
#include "proc.h"
#include "defs.h"
#include "memlayout.h"
#include <stdbool.h>

// the UART control registers are memory-mapped
// at address UART0. this macro returns the
// address of one of the registers.
#define Reg(reg) ((volatile unsigned char *)(UART0 + reg))

// the UART control registers.
// some have different meanings for
// read vs write.
// see http://byterunner.com/16550.html
#define RHR 0                 // receive holding register (for input bytes)
#define THR 0                 // transmit holding register (for output bytes)

#define IER 1                 // interrupt enable register
#define IER_RX_ENABLE (1<<0)  // enable the receiver ready interrupt. 
#define IER_TX_ENABLE (1<<1)  // enable the transmitter empty interrupt.

#define FCR 2                        // FIFO control register
#define FCR_FIFO_ENABLE (1<<0)       // FIFO enable
#define FCR_FIFO_CLEAR (1<<1 | 1<<2) // clear the content of the two FIFOs

#define ISR 2                 // interrupt status register
#define ISR_TXRDY (1<<1)      // THR Empty
#define ISR_RXRDY (1<<2)      // Received Data Ready

#define LCR 3                 // line control register
#define LCR_EIGHT_BITS (3<<0)
#define LCR_BAUD_LATCH (1<<7) // special mode to set baud rate

#define LSR 5                 // line status register: status of data transfer to CPU
#define LSR_RX_READY (1<<0)   // input is waiting to be read from RHR
#define LSR_TX_IDLE (1<<5)    // THR can accept another character to send

#define ReadReg(reg) (*(Reg(reg)))
#define WriteReg(reg, v) (*(Reg(reg)) = (v))

// circular in-memory queue
#define UART_TX_BUF_SIZE 32
char uart_tx_buf[UART_TX_BUF_SIZE];
uint64 uart_tx_w; // write next to uart_tx_buf[uart_tx_w % UART_TX_BUF_SIZE]
uint64 uart_tx_r; // read next from uart_tx_buf[uart_tx_r % UART_TX_BUF_SIZE]
// struct spinlock uart_tx_lock; // lock for protecting queue

void uart_start_tx();

bool isfull(){
    return uart_tx_w == (uart_tx_r + UART_TX_BUF_SIZE);
}

bool isempty(){
    return uart_tx_w == uart_tx_r;
}

void enque(char c){
    uart_tx_buf[uart_tx_w % UART_TX_BUF_SIZE] = c;
    uart_tx_w++;
    //TODO: wake up proc sleeping on the uart_tx_r to read char from queue
}

char deque(){
    char c = uart_tx_buf[uart_tx_r % UART_TX_BUF_SIZE];
    uart_tx_r++;
    // TODO: wake up proc sleeping on uart_tx_w to write char into queue
    return c;
}

void uartinit(){
    // disable interrupt
    WriteReg(IER, 0x00);

    // set baud rate
    WriteReg(LCR, LCR_BAUD_LATCH);
    WriteReg(0, 0x03);
    WriteReg(1, 0x00);
    WriteReg(LCR, LCR_EIGHT_BITS);

    // enable FIFO queue and reset
    WriteReg(FCR, FCR_FIFO_ENABLE | FCR_FIFO_CLEAR);
    // enable interrupt
    WriteReg(IER, IER_RX_ENABLE | IER_TX_ENABLE);
}

// asynchronously write char
void uart_putc(char c){
    //TODO: acqure uart_tx_lock

    while (isfull()){
        // myproc sleep
    }
    enque(c);
    uart_start_tx();

    // release uart_tx_lock
}

// void uart_putc_sync(char c){}

// read one char from UART
char uart_getc(){
    if((ReadReg(LSR) & LSR_RX_READY) != 0){
        // input data is ready.
        char c = ReadReg(RHR);
        return c;
    }
    return -1;
}

// deque char and add to THR for transmit
void uart_start_tx(){
    while(1){
        if(isempty() || (ReadReg(LSR) & LSR_TX_IDLE) == 0){
            // Nothing to transmit, or THR is busy.
            // Will try again later when THR is avialable and fire an intr
            return;
        }

        char c = deque();
        // TODO: wake up a proc sleeping for a space to enque char 
        WriteReg(THR, c);
    }
}

void uartintr(){
    int cause = ReadReg(ISR);
    if (cause){
    }
    while(1){
        char c = uart_getc();
        if (c == -1)
            break;
        // consoleintr(c); // echo received char to console
    }
    // 
    uart_start_tx();
}