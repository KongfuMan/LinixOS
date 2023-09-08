#include "param.h"
#include "types.h"
#include "riscv.h"
#include "proc.h"
#include "fs.h"
#include "buf.h"
#include "defs.h"
#include <stdbool.h>


#define CONSOLE_BUF_SIZE 32
char cons_tx_buf[CONSOLE_BUF_SIZE];
int con_tx_r;
int con_tx_w;

// bool isfull(){
//     return con_tx_r + CONSOLE_BUF_SIZE == con_tx_w;
// }

// void enque(char c){
//     cons_tx_buf[con_tx_w % CONSOLE_BUF_SIZE] = c;
//     con_tx_w++;
//     // wake proc sleeping on con_tx_w waiting to read
// }

// char deque(){
//     char c = cons_tx_buf[con_tx_r % CONSOLE_BUF_SIZE];
//     con_tx_r++;
//     // wake proc sleeping on con_tx_r waiting to write
//     return c;
// }

void consoleinit(){

    uartinit();
}

// read `n` bytes from buffer to `dst` of user space
void consoleread(int user_dst, uint64 dst, int n){
    while(n > 0){
        while(con_tx_r == con_tx_w){
            //myproc sleep
        }

        // char c = deque();
        // copy char c to user space;
    }
}

// write `n` bytes from `src` of user space
void consolewrite(int user_dst, uint64 src, int n){
    
}

// called by uart interrupt handler to echo char to console
void consoleintr(char c){

}