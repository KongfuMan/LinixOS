#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "proc.h"
#include "fs.h"
#include "buf.h"
#include "defs.h"

void plicinit(){
    *(uint32*)(PLIC + UART0_IRQ*4) = 1;
    *(uint32*)(PLIC + VIRTIO0_IRQ*4) = 1;
}

void plicinithart(){
    int hart = cpuid();

    // enable uart/virtio irq for this hart's S-mode.
    *(uint32*)PLIC_SENABLE(hart) = (1 << UART0_IRQ) | (1 << VIRTIO0_IRQ);

    // set this hart's S-mode priority threshold to 0.
    *(uint32*)PLIC_SPRIORITY(hart) = 0;
}