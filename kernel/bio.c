#include "param.h"
#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"
#include "buf.h"
#include "defs.h"

// in memory block buffer manager
struct {
    struct buf buf[NBUF];
    struct buf* freelist;  // circular doubly-linkedlist of free bufs.
    struct buf* lru;        // lru replacer keeps unpinned bufs to be evicted for re-use.
    struct buf* hash_table[NHASH]; // mapping from blockno to index of buf in buf array.
}bcache;

#define _hashfn(dev, blockno) (((unsigned)(dev^blockno))%NHASH)
#define hash(dev, blockno) bcache.hash_table[_hashfn(dev, blockno)]

void insert_freelist(struct buf *b){
    // insert into freelist
    struct buf *p = bcache.freelist->next;
    p->prev = b;
    b->prev = bcache.freelist;
    bcache.freelist->next = b;
    b->next = p;
}

void remove(struct buf *b){
    struct buf *p = b->prev;
    struct buf *n = b->next;
    p->next = n;
    n->prev = p;
}

struct buf* get_hash_table(uint dev, uint blockno){
    struct buf *b;
    for (b = hash(dev, blockno); b != 0; b = b->next){
        if (b->dev == dev && b->blockno == blockno){
            return b;
        }
    }

    return 0;
}

void binit(){
    struct buf *b;
    bcache.freelist->prev = bcache.freelist;
    bcache.freelist->next = bcache.freelist;

    bcache.lru->prev = bcache.lru;
    bcache.lru->next = bcache.lru;

    for (b = bcache.buf; b < bcache.buf + NBUF; b++){
        // init fields of buf
        b->valid = 0;
        b->dirty = 0;
        b->dev = 2;
        b->blockno = 0; // free frame
        b->refcnt = 0;
        insert_freelist(b);
    }
}

struct buf *find_available(uint dev, uint blockno){
    struct buf *b;
    // get from free list => put to hash_map
    if (bcache.freelist->next != bcache.freelist)
    {
        // from free list and remove 
        b = bcache.freelist->next;
        remove(b); //remove from free list
        bpin(b);
        return b;
    }
    
    // evict a victim from lru replacer
    if (bcache.lru->prev != bcache.lru)
    {
        b = bcache.lru->prev;
        remove(b);
        bpin(b);
        return b;
    }

    return 0;
}

static struct buf *bget(uint dev, uint blockno){
    struct buf *b;
    b = get_hash_table(dev, blockno);
    if (b){
        bpin(b);
        return b;
    }

    
    return 0;
}

// read data by <dev, blockno> into b->data using disk driver
struct buf *bread(uint dev, uint blockno)
{
    struct buf *b = bget(dev, blockno);
    if (b)
    {
        virtio_disk_rw(b, 0);
        return b;
    }
    return 0;
}

void bflush(struct buf *b){
    if (b->dirty)
    {
        virtio_disk_rw(b, 1);
        b->dirty = 0;
    }
}

// mark the buf as invalid, s.t. it can be reclaimed later.
void brelse(struct buf *b){
    if (!b){
        return;
    }

    bunpin(b);
    if (b->refcnt == 0){
        b->dev = 0;
        b->blockno = 0;
        struct buf *p = b->prev;
        struct buf *n = b->next;
        p->next = n;
        n->prev = p;
    }
}

void bpin(struct buf *b){
    b->refcnt++;
}

void bunpin(struct buf *b){
    if (b->refcnt <= 0){
        panic("unpin: refcnt is 0.");
    }
    b->refcnt--;
}