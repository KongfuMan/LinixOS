#include "param.h"
#include "types.h"
#include "riscv.h"
#include "proc.h"
#include "fs.h"
#include "buf.h"
#include "defs.h"

void check_page_content(void* pa, int expected){
    char* cpg = (char*)pa;
    int i = 0;
    for (; i < PGSIZE; i++){
        if(cpg[i] != expected){
            panic("test failed");
        }
    }
}

void test_alloc_dealloc(){
    void* pg = kalloc();
    check_page_content(pg, 1);
    memset(pg, 10, PGSIZE);
    check_page_content(pg, 10);
    memset(pg, 20, PGSIZE);
    check_page_content(pg, 20);
    kfree(pg);
}

void test_virtio_rw(){
    struct buf *bw = (struct buf*)kalloc();
    bw->blockno = 100;
    for (int i = 0; i < BSIZE; i++){
        bw->data[i] = 'a';
    }
    virtio_disk_rw(bw, 1);
    // struct buf *br = (struct buf*)kalloc();
    // bw->blockno = 100;
    // virtio_disk_rw(br, 0);
}

volatile int starting = 1;

void main(){
    if (starting == 1){
        consoleinit();
        // printfinit();
        printf("\n");
        printf("xv6 from scratch kernel is booting\n");
        printf("\n");
        kinit();            // init physical page allocator
        kvminit();          // create kernel page table.
        kvminithart();      // enable paging for each hart.
        procinit();         // process table
        trapinit();         // trap vectors
        trapinithart();     // install kernel trap vector
        plicinit();         // set up interrupt controller
        plicinithart();     // ask PLIC for device interrupts
        // binit();         // buffer cache
        // iinit();         // inode table
        // fileinit();      // file table
        virtio_disk_init(); // emulated hard disk
        userinit();         // first user process
        __sync_synchronize();
        starting = 0;
    }else{
        while(starting);
        __sync_synchronize();
        kvminithart();
        trapinithart();
        plicinithart();
    }

    printf("hart %d initialization complete. \n", cpuid());
    printf("call scheduler() to context switch into the first proc. \n");
    scheduler(); 
}