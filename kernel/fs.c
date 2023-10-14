// inode and file system

struct superblock sb;

void iinit(){

}


void fsinit(){
    
}

// alloc a free block.
// return the block number. 0 if running out of disk block
static uint
balloc(uint dev){
    return 0;
}

// Free a disk block.
static void
bfree(int dev, uint blockno){

}

// get or create physical block number mapped to virtual block number
int 
bmap(inode* ip, int bn){
    return 0;
}