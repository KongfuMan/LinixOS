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

struct superblock sb;

// An inode describes a single unnamed file.
// The inode disk structure holds metadata: the file's type,
// its size, the number of links referring to it, and the
// list of blocks holding the file's content.
//
// The inodes are laid out sequentially on disk at block
// sb.inodestart. Each inode has a number, indicating its
// position on the disk.
//
// The kernel keeps a table of in-use inodes in memory
// to provide a place for synchronizing access
// to inodes used by multiple processes. The in-memory
// inodes include book-keeping information that is
// not stored on disk: ip->ref and ip->valid.
//
// An inode and its in-memory representation go through a
// sequence of states before they can be used by the
// rest of the file system code.
//
// * Allocation: an inode is allocated if its type (on disk)
//   is non-zero. ialloc() allocates, and iput() frees if
//   the reference and link counts have fallen to zero.
//
// * Referencing in table: an entry in the inode table
//   is free if ip->ref is zero. Otherwise ip->ref tracks
//   the number of in-memory pointers to the entry (open
//   files and current directories). iget() finds or
//   creates a table entry and increments its ref; iput()
//   decrements ref.
//
// * Valid: the information (type, size, &c) in an inode
//   table entry is only correct when ip->valid is 1.
//   ilock() reads the inode from
//   the disk and sets ip->valid, while iput() clears
//   ip->valid if ip->ref has fallen to zero.
//
// * Locked: file system code may only examine and modify
//   the information in an inode and its content if it
//   has first locked the inode.
//
// Thus a typical sequence is:
//   ip = iget(dev, inum)
//   ilock(ip)
//   ... examine and modify ip->xxx ...
//   iunlock(ip)
//   iput(ip)
//
// ilock() is separate from iget() so that system calls can
// get a long-term reference to an inode (as for an open file)
// and only lock it for short periods (e.g., in read()).
// The separation also helps avoid deadlock and races during
// pathname lookup. iget() increments ip->ref so that the inode
// stays in the table and pointers to it remain valid.
//
// Many internal file system functions expect the caller to
// have locked the inodes involved; this lets callers create
// multi-step atomic operations.
//
// The itable.lock spin-lock protects the allocation of itable
// entries. Since ip->ref indicates whether an entry is free,
// and ip->dev and ip->inum indicate which i-node an entry
// holds, one must hold itable.lock while using any of those fields.
//
// An ip->lock sleep-lock protects all ip-> fields other than ref,
// dev, and inum.  One must hold ip->lock in order to
// read or write that inode's ip->valid, ip->size, ip->type, &c.

struct {
    struct spinlock lock;
    struct inode inode[NINODE];
} itable;

static void
readsb(int dev, struct superblock *sb);

void iinit(){
    // initialize the itable
    initlock(&itable.lock, "itable spin");
    for (int i = 0; i < NINODE; i++){
        initsleeplock(&itable.inode[i].lock, "inode sleep");
        itable.inode[i].inum = 0;
        itable.inode[i].type = T_NONE;
    }
}

static struct inode*
iget(uint dev, uint inum);

// Allocate an empty inode on device dev.
// Mark it as allocated by giving it type type.
// Returns an unlocked but allocated and referenced inode,
// or NULL if there is no free inode.
struct inode* ialloc(uint dev, short type){
    // load the inode from disk to check if it is free
    int inum;
    struct buf *b;
    struct dinode *dip;

    for (inum = 0; inum < sb.ninodes; inum += IPB){
        b = bread(dev, IBLOCK(inum, sb));
        for (dip = (struct dinode*)b->data; dip < ((struct dinode*)b->data + IPB); dip++){
            if (dip->type == T_NONE){
                memset(dip, 0, sizeof(dip));
                // update dinode type to mark it as allocated.
                dip->type = type;
                bwrite(b);
                brelse(b);
                // find available inode struct for inum
                return iget(dev, inum);
            }
        }
        brelse(b);
    }

    printf("no free inode avaiable");
    return 0;
}

// Find the inode with number inum on device dev
// and return the in-memory copy. Does not lock
// the inode and does not read it from disk.
static struct inode*
iget(uint dev, uint inum){
    struct inode *ip, *empty;
    acquire(&itable.lock);

    empty = 0;
    for (ip = &itable.inode[0]; ip < &itable.inode[NINODE]; ip++){
        if (ip->ref > 0 && ip->dev == dev && ip->inum == inum){
            // already cached
            ip->ref++;
            release(&itable.lock);
            return ip;
        }

        if (empty == 0 && ip->ref == 0){
            // first empty inode
            empty = ip;
            break;
        }
    }

    if (empty == 0){
        printf("");
        return 0;
    }

    // find a free inode to use
    ip->dev = dev;
    ip->inum = inum;
    ip->ref = 1;
    release(&itable.lock);
    return ip;
}

// struct inode* idup(struct inode*);
// void ilock(struct inode*);
// void iput(struct inode*);
// void iunlock(struct inode*);
// void iunlockput(struct inode*);
// void iupdate(struct inode*);
// int readi(struct inode*, int, uint64, uint, uint);
// void stati(struct inode*, struct stat*);
// int writei(struct inode*, int, uint64, uint, uint);
// void itrunc(struct inode*);

// Read the super block.
static void
readsb(int dev, struct superblock *sb){
    struct buf *bp;
    bp = bread(dev, 1);
    memmove(sb, bp->data, sizeof(struct superblock));
    brelse(bp);
}

// fs operation
void fsinit(int dev){
    readsb(dev, &sb);
    if(sb.magic != FSMAGIC){
        panic("invalid file system");
    }
}

// alloc a free block.
// return the block number. 0 if running out of disk block
// static uint
// balloc(uint dev){
//     return 0;
// }

// Free a disk block.
// static void
// bfree(int dev, uint blockno){
// }

// get or create physical block number mapped to virtual block number
// int 
// bmap(inode* ip, int bn){
//     return 0;
// }

// directory operation
int             dirlink(struct inode*, char*, uint);
struct inode*   dirlookup(struct inode*, char*, uint*);
int             namecmp(const char*, const char*);
struct inode*   namei(char*);
struct inode*   nameiparent(char*, char*);