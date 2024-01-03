#include "param.h"
#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "fs.h"
#include "buf.h"
#include "net.h"
#include "defs.h"
#include "file.h"

struct devsw devsw[NDEV];
struct {
    struct spinlock lock;
    struct file file[NFILE];
}ftable;

struct file* filealloc(){
    // loop to find the an avaiable file struct from file table
    struct file *f;
    acquire(&ftable.lock);
    for (f = ftable.file; f < ftable.file + NFILE; f++){
        if (f->ref == 0){
            f->ref = 1;
            release(&ftable.lock);
            return f;
        }
    }

    release(&ftable.lock);
    return 0;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void fileclose(struct file* f){
    panic("fileclose not implemented");
    struct file ff;

    acquire(&ftable.lock);
    if(f->ref < 1)
        panic("fileclose");
    if(--f->ref > 0){
        release(&ftable.lock);
        return;
    }
    ff = *f;
    f->ref = 0;
    f->type = FD_NONE;
    release(&ftable.lock);

    if(ff.type == FD_PIPE){
        // pipeclose(ff.pipe, ff.writable);
    } else if(ff.type == FD_INODE || ff.type == FD_DEVICE){
        // begin_op();
        iput(ff.ip);
        // end_op();
    }
}

struct file* filedup(struct file* f){
    acquire(&ftable.lock);
    if (f->ref < 1){
        panic("filedup: cannot dup a freed file");
    }
    f->ref++;
    release(&ftable.lock);
    return f;
}

void fileinit(void){
    initlock(&ftable.lock, "ftable");
    acquire(&ftable.lock);
    struct file *f;
    for (f = ftable.file; f < ftable.file + NFILE; f++){
        f->ref = 0;
    }
    release(&ftable.lock);
}

// write `n` bytes from file f to user virtual address `addr`
// return actual number of bytes read.
int fileread(struct file* f, uint64 uva_dst, int n){
    // 1. copy from user space to kernel space
    // 2. write to file.
    if (f->type == FD_DEVICE){
        if (f->major < 0 || f->minor > NDEV || devsw[f->major].read == 0){
            return -1;
        }
        return devsw[f->major].read(1, uva_dst, n);
    } else if (f->type == FD_INODE){
        // readi();
    }
    return 0;
}

int filestat(struct file* file, uint64 addr){
    return 0;
}

// write `n` bytes from file f to user virtual address `addr`
// return actual number of bytes written.
int filewrite(struct file* file, uint64 addr, int n){
    return 0;
}