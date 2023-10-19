#include "param.h"
#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "fs.h"
#include "buf.h"
#include "defs.h"

// in memory block buffer manager
struct {
    struct buf buf[NBUF];
    struct buf* freelist;    // dummy head of circular doubly-linkedlist of free bufs.
    struct buf* lru;         // dummy head of circular doubly-linkedlist of lru replacer of unpinned buf to be evicted for re-use.
    struct buf* hash_table[NHASH]; // mapping from blockno to index of buf in buf array.
}bcache;

#define _hashfn(dev, blockno) (((unsigned)(dev^blockno))%NHASH)
#define hash(dev, blockno) bcache.hash_table[_hashfn(dev, blockno)]

void remove_from_freelist(struct buf *b){
    struct buf *p = b->prev_free;
    struct buf *n = b->next_free;
    if (!p || !n){
        panic("freelist buf corrupted.");
    }
    p->next_free = n;
    n->prev_free = p;

    b->next_free = b->prev_free = 0;
}

void remove_from_lru(struct buf *b){
    struct buf *p = b->prev_lru;
    struct buf *n = b->next_lru;
    if (!p || !n){
        panic("lru buf corrupted.");
    }
    p->next_lru = n;
    n->prev_lru = p;

    b->next_lru = b->prev_lru = 0;
}

void remove_from_hashqueue(struct buf *b){
    // case1: b is the head
    // case2: b is the tail
    // case3: b is in the middle
    struct buf *p = b->prev_hash;
    struct buf *n = b->next_hash;
    if (p){
        p->next_hash = n;
    }
    if (n){
        n->prev_hash = p;
    }

    // update head if b is head
    if (b == hash(b->dev, b->blockno)){
        hash(b->dev, b->blockno) = b->next_hash;
    }

    b->next_hash = b->prev_hash = 0;
}

void put_last_freelist(struct buf *b){
    if (!b){
        return;
    }

    struct buf *p = bcache.freelist->next_free;
    p->prev_free = b;
    b->prev_free = bcache.freelist;
    bcache.freelist->next_free = b;
    b->next_free = p;
}

void put_last_lru(struct buf *b){
    if (!b){
        return;
    }

    struct buf *p = bcache.lru->next_lru;
    p->prev_lru = b;
    b->prev_lru = bcache.lru;
    bcache.lru->next_lru = b;
    b->next_lru = p;
}

void put_last_hashqueue(struct buf *b){
    struct buf *head = hash(b->dev, b->blockno);
    if (!head){
        hash(b->dev, b->blockno) = b;
        return;
    }

    struct buf *p = head->next_hash;
    p->prev_hash = b;
    b->prev_hash = head;
    head->next_hash = b;
    b->next_hash = p;
}

// hash_table.get(key)
struct buf* get_hash_table(uint dev, uint blockno){
    struct buf *b;
    for (b = hash(dev, blockno); b != 0; b = b->next_hash){
        if (b->dev == dev && b->blockno == blockno){
            return b;
        }
    }

    return 0;
}

void binit(){
    struct buf *b;
    // since kernel virtual mem has been vertically mapped into physical mem by adding ptes in kernel page table
    // so there is no need to do kvmmap again here.
    bcache.freelist = (struct buf*)kalloc();
    bcache.freelist->prev_free = bcache.freelist;
    bcache.freelist->next_free = bcache.freelist;

    bcache.lru = (struct buf*)kalloc();
    bcache.lru->prev_lru = bcache.lru;
    bcache.lru->next_lru = bcache.lru;

    for (b = bcache.buf; b < bcache.buf + NBUF; b++){
        // init fields of buf
        b->valid = 0;
        b->dirty = 0;
        b->dev = 2;
        b->blockno = 0;
        b->refcnt = 0;
        put_last_freelist(b);
    }
}

// find a buf from freelist first, then from replacer if no free buf available.
struct buf *find_available(uint dev, uint blockno){
    struct buf *b;
    // get from free list => put to hash_map
    if (bcache.freelist->next_free != bcache.freelist)
    {
        // from free list and remove 
        b = bcache.freelist->next_free;
        remove_from_freelist(b); //remove from free list
        bpin(b);
        return b;
    }
    
    // evict a victim(least recently used) from lru replacer
    if (bcache.lru->next_lru != bcache.lru){
        b = bcache.lru->prev_lru;
        remove_from_lru(b);
        remove_from_hashqueue(b);
        bpin(b);
        return b;
    }

    return 0;
}

// fetch a buf for use.
static struct buf *bget(uint dev, uint blockno){
    struct buf *b;
    b = get_hash_table(dev, blockno);
    if (b){
        // remove from lru if exist.
        if (b->refcnt == 0){
            remove_from_lru(b);
        }
        bpin(b);
        return b;
    }

    // either get buf from a freelist or lru
    b = find_available(dev, blockno);
    if (!b){
        return 0;
    }
    
    b->dev = dev;
    b->blockno = blockno;
    put_last_hashqueue(b);
    return b;
}

// read data by <dev, blockno> into b->data using disk driver
struct buf *bread(uint dev, uint blockno){
    struct buf *b = bget(dev, blockno);
    if (b){
        virtio_disk_rw(b, 0);
        return b;
    }
    return 0;
}

// data will be flushed
void bwrite(struct buf *b){
    if (b->dirty){
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
        // add to lru to reuse
        put_last_lru(b);
    }
}

void bfree(struct buf *b){
    
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