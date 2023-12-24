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
// The itable.lock (spin-lock) protects the allocation of itable
// entries. Since ip->ref indicates whether an entry is free,
// and ip->dev and ip->inum indicate which i-node an entry
// holds, one must hold itable.lock while using any of those fields.
//
// An ip->lock (sleep-lock) protects all ip->* fields other than ref,
// dev, and inum.  One must hold ip->lock in order to
// read or write that inode's ip->valid, ip->size, ip->type, &c.

struct {
    struct spinlock lock;
    struct inode inode[NINODE];
} itable;

extern struct superblock sb;

// initialzie inode table
void iinit(){
    // initialize the itable
    initlock(&itable.lock, "itable spin");
    for (int i = 0; i < NINODE; i++){
        initsleeplock(&itable.inode[i].lock, "inode sleep");
        itable.inode[i].inum = 0;
        itable.inode[i].type = T_NONE;
    }
}

// Find the inode with number inum on device dev
// and return the in-memory copy. Does not lock
// the inode and does not read it from disk.
struct inode*
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
    ip->valid = 0; // set invalid since ip->data still yet to be loaded
    release(&itable.lock);
    return ip;
}

// Allocate an empty inode on device dev.
// Mark it as allocated by giving it type type.
// Returns an unlocked but allocated and referenced inode,
// or NULL if there is no free inode.
struct inode*
ialloc(uint dev, short type){
    // load the inode from disk to check if it is free
    int inum;
    struct buf *b;
    struct dinode *dip;

    for (inum = ROOTINO; inum < sb.ninodes; inum++){
        b = bread(dev, IBLOCK(inum, sb));
        dip = (struct dinode*)b->data + inum % IPB;
        if (dip->type == T_NONE){
            memset(dip, 0, sizeof(dip));
            // update dinode type to mark it as allocated.
            dip->type = type;
            bwrite(b);
            brelse(b);
            // find available inode struct for inum
            return iget(dev, inum);
        }
        brelse(b);
    }

    printf("no free inode avaiable");
    return 0;
}

// Increment reference count for ip.
// Returns ip to enable ip = idup(ip1) idiom.
struct inode*
idup(struct inode *ip)
{
  acquire(&itable.lock);
  ip->ref++;
  release(&itable.lock);
  return ip;
}

// Read `n` bytes data starting from `off` from inode into dst.
// Caller must hold ip->lock.
// If user_dst==1, then dst is a user virtual address;
// otherwise, dst is a kernel address.
// return the actual bytes read.
int
readi(struct inode *ip, int user_dst, uint64 dst, uint off, uint n){
    // arguments check
    if (!ip || (off + n) > ip->size ||  n < 0){
        return -1;
    }

    struct buf *bp;
    uint tot, sz; // actual bytes read totally
    uint blockno;
    for (tot = 0; tot < n; tot += sz){
        blockno = bmap(ip, (uint)(off / BSIZE)); // read block by blockno
        bp = bread(ip->dev, blockno);
        if (!bp){
            return 0;
        }

        sz = min(BSIZE - off % BSIZE, n - tot); // actual bytes read for one iteration
        either_copyout(user_dst, dst, &bp->data[off % BSIZE], sz);
        dst += sz;
        off += sz;
    }
    
    return tot;
}

// write `n` bytes starting from off
int writei(struct inode* ip, int user_src, uint64 src, uint off, uint n){
    if(!ip || (off + n) > ip->size ||  n < 0 ){
        return -1;
    }

    uint tot, size, offset;
    uint blockno;
    struct buf *bp;
    for (tot = 0; tot < n; tot += size){
        blockno = bmap(ip, (uint)(off / BSIZE)); // get actual blockno or create if not exist
        if (blockno == 0){
            break;
        }

        bp = bread(ip->dev, blockno);
        offset = off % BSIZE;   // offset within block
        size = min(BSIZE - offset, n - tot);
        if (either_copyin((void *)(&bp->data[offset]), user_src, src, size) == -1){
            brelse(bp);
            break;
        }

        bwrite(bp);
        brelse(bp);
        off += size;
        src += size;
    }

    return tot;
}

// load inode data from disk if not loaded yet and acquire inode sleep lock
void ilock(struct inode* ip){
    struct buf *bp;
    struct dinode *dip;

    if(ip == 0 || ip->ref < 1){
        panic("ilock");
    }

    acquiresleep(&ip->lock);

    if (!ip->valid){ //inode data has not been loaded from disk
        bp = bread(ip->dev, IBLOCK(ip->inum, sb));
        dip = (struct dinode*)bp->data + (ip->inum % IPB);
        ip->type = dip->type;
        ip->major = dip->major;
        ip->minor = dip->minor;
        ip->nlink = dip->nlink;
        ip->size = dip->size;
        memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
        brelse(bp);
        ip->valid = 1;
        if(ip->type == 0){
            panic("ilock: no type");
        }
    }
}

// Unlock the given inode.
void iunlock(struct inode* ip){
    if(ip == 0 || !holdingsleep(&ip->lock) || ip->ref < 1){
        panic("iunlock: not holding the lock");
    }

    releasesleep(&ip->lock);
}

// Drop a reference to an in-memory inode.
// If that was the last reference, the inode table entry can
// be recycled.
// If that was the last reference and the inode has no links
// to it, free the inode (and its content) on disk.
// All calls to iput() must be inside a transaction in
// case it has to free the inode.
void iput(struct inode* ip){
    acquire(&itable.lock);

    if(ip->ref == 1 && ip->valid && ip->nlink == 0){
        // inode has no links and no other references: truncate and free.

        // ip->ref == 1 means no other process can have ip locked,
        // so this acquiresleep() won't block (or deadlock).
        acquiresleep(&ip->lock);

        release(&itable.lock);

        itrunc(ip);
        ip->type = 0;
        iupdate(ip);
        ip->valid = 0;

        releasesleep(&ip->lock);

        acquire(&itable.lock);
    }

    ip->ref--;
    release(&itable.lock);
}

void iunlockput(struct inode* ip){
    iunlock(ip);
    iput(ip);
}

// Copy a modified in-memory inode to disk.
// Must be called after every change to an inode's field
// that lives on disk.
// Caller must hold ip->lock.
void iupdate(struct inode* ip){
    if (!ip || !holdingsleep(&ip->lock)){
        panic("iupdate: not holding sleep lock");
    }

    struct buf *bp;
    struct dinode *dip;

    bp = bread(ip->dev, IBLOCK(ip->inum, sb));
    dip = (struct dinode*)bp->data + (ip->inum % IPB);
    dip->type = ip->type;
    dip->major = ip->major;
    dip->minor = ip->minor;
    dip->nlink = ip->nlink;
    dip->size = ip->size;
    memmove(dip->addrs, ip->addrs, NDIRECT+1);
    bwrite(bp);
    brelse(bp);
}

// Copy stat information from inode.
// Caller must hold ip->lock.
void stati(struct inode*, struct stat*){
}

// Truncate inode (discard contents).
// Caller must hold ip->lock.
void itrunc(struct inode* ip){
    if (!ip || !holdingsleep(&ip->lock)){
        panic("itrunc");
    }

    if (ip->valid == 0){
        return;
    }

    int i, j;
    struct buf *bp;
    uint *a;

    // free all blocks indexed by inode
    for(i = 0; i < NDIRECT; i++){
        if(ip->addrs[i]){
            bfree(ip->dev, ip->addrs[i]);
            ip->addrs[i] = 0;
        }
    }

    if(ip->addrs[NDIRECT]){
        bp = bread(ip->dev, ip->addrs[NDIRECT]);
        a = (uint*)bp->data;
        for(j = 0; j < NINDIRECT; j++){
            if(a[j]){
                bfree(ip->dev, a[j]);
            }
        }
        brelse(bp);
        bfree(ip->dev, ip->addrs[NDIRECT]);
        ip->addrs[NDIRECT] = 0;
    }

    ip->size = 0;

    // save the inode info on disk.
    iupdate(ip);
}
