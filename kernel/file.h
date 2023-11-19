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
    // immutable fields, thus thread safe
    uint dev;               // device number
    uint inum;              // inode number

    struct sleeplock lock;  // sleep lock protecting the fields below.
    int ref;
    short type;              // device type
    int size;                // size in bytes used 
    uint addrs[NDIRECT + 1]; // index is logic blockno => addr[index] is the pyhsical blockno.
};