// an open file
struct file {
    enum { FD_NONE, FD_PIPE, FD_INODE, FD_DEVICE } type;
    int ref; // reference count
    char readable;
    char writable;
    struct pipe *pipe; // FD_PIPE
    struct inode *ip;  // FD_INODE and FD_DEVICE
    uint off;          // FD_INODE
    short major;       // FD_DEVICE
    short minor;

    // struct sock *sock; // for socket
};

#define major(dev)  ((dev) >> 16 & 0xFFFF)
#define minor(dev)  ((dev) & 0xFFFF)
#define	mkdev(m,n)  ((uint)((m)<<16| (n)))

// node that indexing a bunch of blocks for a file. It represents a continuous logic disk space of a file.
// inode represent the in-memory data structure. 
// dinode reprensets the deserialization schema of inode on disk 
struct inode {
    // immutable fields once inode is created, thus thread safe
    uint dev;               // device number
    uint inum;              // inode number

    struct sleeplock lock;  // sleep lock protecting the fields below.
    int ref;
    int valid;              // inode has been read from disk?

    short type;             // copy of disk inode
    short major;
    short minor;
    short nlink;
    int size;                // size in bytes used 
    uint addrs[NDIRECT + 1]; // index is logic blockno => addr[index] is the pyhsical blockno.
};

struct devsw {
    // function pointer, similar as interface
    int (*read)(int, uint64, int);
    int (*write)(int, uint64, int);
};

extern struct devsw devsw[NDEV];

#define CONSOLE 1