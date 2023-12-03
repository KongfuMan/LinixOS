// an open file
struct file {
    int ref;
    uint offset;
    struct inode *ip;  // for device
    char readable;
    char writable;
    short major;    // device
    // struct sock *sock; // for socket
    // struct pipe *pipe; // for pipe
};

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

#define CONSOLE 1