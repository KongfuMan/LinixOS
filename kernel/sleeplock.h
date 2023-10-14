struct sleeplock{
    // spin lock to protect the fields
    uint locked;       // Is the lock held?
    struct spinlock lk; // spinlock protecting this sleep lock
    char *name;        // Name of lock.
    int pid;           // Process holding lock
};