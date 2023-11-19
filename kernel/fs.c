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


// Zero a block.
static void
bzero(int dev, int blockno)
{
  struct buf *bp;

  bp = bread(dev, blockno);
  memset(bp->data, 0, BSIZE);
  bwrite(bp);
  brelse(bp);
}

// alloc a free block.
// return the block number. 0 if running out of disk block
uint
balloc(uint dev){
    int b, bi, mask;
    struct buf *bp = 0;

    // sb.size is the total # of fs block
    for(b = 0; b < sb.size; b += BPB){ // starting bit number of each fs block
        bp = bread(dev, BBLOCK(b, sb));
        for (bi = 0; bi < BPB && (b + bi) < sb.size; bi++){  // 0-based bit number inside each data block
            mask = 1<<(bi % 8);    // mask of a byte: left shift 1 by `bi` bit
            if (!(bp->data[bi/8] & mask)){
                // free block of blockno `b`
                bp->data[bi/8] |= mask;  // flip bit from 0 to 1.
                bwrite(bp);
                brelse(bp);
                bzero(dev, b + bi);
                return b + bi;
            }
        }
        brelse(bp);
    }
    printf("balloc: out of blocks\n");
    return 0;
}

// free a data block with pyhsical blockno (starting from `datastart`) by simply flip the corresponding bit
void
bfree(int dev, uint blockno){
    // blockno => bit #
    int bmpb = BBLOCK(blockno, sb);
    int bitb = blockno % BPB;

    struct buf *bp;
    bp = bread(dev, bmpb);
    int mask = 1 << (bitb % 8);
    if ((bp->data[bitb/8] & mask) == 0){
        printf("Cannot deallocate a free block");
        return;
    }

    bp->data[bitb/8] &= ~mask; // flip bit from 1 to 0
    bwrite(bp);
    brelse(bp);
}

// Return the disk block address of the nth block in inode ip.
// If there is no such block, bmap allocates one.
// returns 0 if out of disk space.
// `bn` is the 0-based logic blockno per inode
uint 
bmap(struct inode* ip, int bn){
    if (!ip || bn < 0 || bn >= (NDIRECT + NINDIRECT)){
        panic("bmap: empty inode or logic block no out of range");
    }

    struct buf *bp;
    uint pbn;
    // direct block
    if (bn < NDIRECT){
        if (ip->addrs[bn] != 0){
            return ip->addrs[bn];
        }
        pbn = balloc(ip->dev); //allocate a physical blockno.
        if (!pbn){
            panic("bmap");
        }
        ip->addrs[bn] = pbn;
        return pbn;
    }

    // indirect block
    if (ip->addrs[NDIRECT] == 0){
        pbn = balloc(ip->dev); // alloc a physical block.
        if (!pbn){
            panic("bmap");
        }
        ip->addrs[NDIRECT] = pbn;
    }
    bp = bread(ip->dev, ip->addrs[NDIRECT]);
    uint *nbn = (uint*)bp->data + (bn - NDIRECT); // set the block no
    pbn = balloc(ip->dev);
    if (!pbn){
        panic("bmap");
    }
    *nbn = pbn;
    bwrite(bp);
    brelse(bp);
    return pbn;
}

/******************************** directory operations ********************************/

int
namecmp(const char *s, const char *t)
{
  return strncmp(s, t, DIRSIZE);
}

// Copy the next path element from path into name.
// Return a pointer to the element following the copied one.
// The returned path has no leading slashes,
// so the caller can check *path=='\0' to see if the name is the last one.
// If no name to remove, return 0.
//
// Examples:
//   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
//   skipelem("///a//bb", name) = "bb", setting name = "a"
//   skipelem("a", name) = "", setting name = "a"
//   skipelem("", name) = skipelem("////", name) = 0
//
// static char*
// skipelem(char *path, char *name){
//     char *s;
//     int len;

//     while(*path == '/'){
//         path++;
//     }
//     if(*path == 0){
//         return 0;
//     }
//     s = path;
//     while(*path != '/' && *path != 0){
//         path++;
//     }
//     len = path - s;
//     if(len >= DIRSIZE){
//         memmove(name, s, DIRSIZE);
//     }
//     else {
//         memmove(name, s, len);
//         name[len] = 0;
//     }
//     while(*path == '/'){
//         path++;
//     }
//     return path;
// }

// Write a new directory entry (name, inum) into the directory dp.
// Returns 0 on success, -1 on failure (e.g. out of disk blocks).
int
dirlink(struct inode *dp, char *name, uint inum){
    // link a dirent associated with given path to an inode.

    return -1;
}

// Look for a directory entry in a directory.
// If found, set *poff to byte offset of entry.
struct inode*
dirlookup(struct inode*, char* path, uint* poff);

// Look up and return the inode for a path name.
// If parent != 0, return the inode for the parent and copy the final
// path element into name, which must have room for DIRSIZE bytes.
// Must be called inside a transaction since it calls iput().
static struct inode*
namex(char *path, int nameiparent, char *name){
    // struct inode *ip;

    // if (*path == '/'){
    //     ip = iget(ROOTDEV, ROOTINO); //
    // }else{
    //     ip = idup(current_proc()->cwd);
    // }

    return 0;
}

int
namecmp(const char*, const char*);


// Find inode by path
struct inode*
namei(char* path){
    char name[DIRSIZE];
    return namex(path, 0, name);
}

struct inode*
nameiparent(char*, char*);