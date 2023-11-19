#include "param.h"
#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "fs.h"
#include "buf.h"
#include "defs.h"
#include "file.h"
#include "stat.h"

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

void fill_data(struct buf * b, uchar content){
    for (int i = 0; i < BSIZE; i++){
        b->data[i] = content;
    }
}

void test_buf_w(){
    uint dev = T_FILE;
    struct buf *b;
    uint blockno;
    uchar content = 1;
    for (int i = 100; i < 2000; i++){
        blockno = i;
        b = bread(dev, blockno);
        fill_data(b, content);
        bwrite(b);
        brelse(b);
        content++;
    }
}

void test_buf_r(){
    uint dev = T_FILE;
    struct buf *b;
    uint blockno;
    uchar content = 1;
    for (int i = 100; i < 2000; i++){
        blockno = i;
        b = bread(dev, blockno);
        brelse(b);
        for (int i = 0; i < BSIZE; i++){
            if (b->data[i] != content){
                panic("test_virtio_r: read data error");
            }
        }
        content++;
    }
}

void test_alloc_inode(){
    fsinit(1);
    uint dev = 1;
    uint bn = balloc(dev);
    bfree(dev, bn);
    for (int i = 0; i < 10; i++){
        uint actual_bn = balloc(dev);
        bfree(dev, actual_bn);
        // // struct inode *ip = ialloc(dev, type);
        // // ip = ialloc(1, 10);
        // // ip = ialloc(1, 10);
        // // ip = ialloc(1, 10);
        assert (bn == actual_bn);
    }
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
        binit();            // buffer
        iinit();            // inode table
        // fileinit();      // file table
        virtio_disk_init(); // emulated hard disk
        // test_buf_r();
        test_alloc_inode();
        // pci_init();      // pci initialize
        // sockinit();      // network init
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