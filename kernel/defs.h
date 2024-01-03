/* below are APIs public to kernel. */

struct proc;
struct cpu;
struct lock;
struct spinlock;
struct buf;
struct inode;
struct file;
struct stat;
struct sock;

// console.c
void consoleinit(void);
void consoleintr(int);
void consputc(int);

// fs.c
void            fsinit(int);
int             dirlink(struct inode*, char*, uint);
struct inode*   dirlookup(struct inode*, char*, uint*);
struct inode*   ialloc(uint, short);
struct inode*   idup(struct inode*);
void            iinit();
void            ilock(struct inode*);
int             namecmp(const char*, const char*);
struct inode*   namei(char*);
struct inode*   nameiparent(char*, char*);

//inode.c
void            iput(struct inode*);
void            iunlock(struct inode*);
void            iunlockput(struct inode*);
void            iupdate(struct inode*);
int             readi(struct inode*, int, uint64, uint, uint);
void            stati(struct inode*, struct stat*);
int             writei(struct inode*, int, uint64, uint, uint);
void            itrunc(struct inode*);
struct inode*   iget(uint, uint);
struct inode*   ialloc(uint, short);
struct inode*   idup(struct inode*);

// function internal to fs.c, declared here just for test, remove
uint balloc(uint dev);
void bfree(int dev, uint blockno);

// file.c
struct file*    filealloc(void);
void            fileclose(struct file*);
struct file*    filedup(struct file*);
void            fileinit(void);
int             fileread(struct file*, uint64, int n);
int             filestat(struct file*, uint64 addr);
int             filewrite(struct file*, uint64, int n);

// uart.c
void            uartinit(void);
void            uartputc(int);
void            uartputc_sync(int);
int             uartgetc(void);
void            uartintr(void);

// kalloc.c
void kinit(void);
void* kalloc(void);
void kfree(void*);
void incr_ref(void*);
void decr_ref(void*);
int get_ref(void*);

// string.c
void* memset(void*, int, uint);
int memcmp(const void*, const void*, uint);
void* memmove(void*, const void*, uint);
void* memset(void*, int, uint);
char* safestrcpy(char*, const char*, int);
int strlen(const char*);
int strncmp(const char*, const char*, uint);
char* strncpy(char*, const char*, int);

// printf.c
void printf(char*, ...);
void panic(char*);
void assert(uint);
void printfinit(void);

// vm.c
void kvminit(void);
void kvmmap(pagetable_t, uint64, uint64, uint64, int);
void kvminithart(void);
int mappages(pagetable_t, uint64, uint64, uint64, int);
void uvmunmap(pagetable_t, uint64, int, int);
pagetable_t uvmcreate(void);
void uvmfirst(pagetable_t, uchar *, uint);
int uvmcopy(pagetable_t, pagetable_t, uint64);
pte_t * walk(pagetable_t, uint64, int);
int copyout(pagetable_t, uint64, char *, uint64);
int copyin(pagetable_t, char *, uint64, uint64);
int copyinstr(pagetable_t, char *, uint64, uint64);
uint64 uvmalloc(pagetable_t, uint64, uint64, int);
uint64 uvmdealloc(pagetable_t, uint64, uint64);
uint64 walkaddr(pagetable_t, uint64);
void uvmclear(pagetable_t, uint64);
void uvmfree(pagetable_t, uint64);

//proc.c
void            proc_mapstacks(pagetable_t);
void            procinit(void);
int             cpuid(void);
void            userinit(void);
void            scheduler(void);
struct cpu*     current_cpu();
struct proc*    current_proc(void);
void            sleep(void*, struct spinlock*);
void            sched(void);
pid_t           fork(void);
void            wakeup(void*);
void            push_off(void);
void            pop_off(void);
int             either_copyout(int user_dst, uint64 dst, void *src, uint64 len);
int             either_copyin(void *dst, int user_src, uint64 src, uint64 len);
pagetable_t     proc_pagetable(struct proc*);
void            proc_freepagetable(pagetable_t, uint64);
int             wait(uint64);
void            exit(int);

//trap.c
void trapinit(void);
void trapinithart(void);
void usertrap(void);
void usertrapret(void);

//plic.c
void plicinit(void);
void plicinithart(void);
int plic_claim(void); 
void plic_complete(int);

//swtch.S
void swtch(struct context *from, struct context *to);

// virtio_disk.c
void virtio_disk_init(void);
void virtio_disk_rw(struct buf *, int);
void virtio_disk_intr(void);

// bio.c
void binit(void);
struct buf *bread(uint, uint);
void brelse(struct buf*);
void bwrite(struct buf*);
void bpin(struct buf*);
void bunpin(struct buf*);
uint bmap(struct inode* ip, int bn);

// syscall.c
void syscall(void);
void argaddr(int, uint64*);
int argstr(int, char*, int);
void argint(int, int*);
int fetchaddr(uint64, uint64*);
int fetchstr(uint64, char*, int);

// sleeplock.c
void acquiresleep(struct sleeplock*);
void releasesleep(struct sleeplock*);
int holdingsleep(struct sleeplock*);
void initsleeplock(struct sleeplock*, char*);

// spinlock.c
void initlock(struct spinlock*, char*);
void acquire(struct spinlock*);
void release(struct spinlock*);
int holding(struct spinlock*);

// exec.c
int exec(char*, char**);

#define NELEM(x) (sizeof(x)/sizeof((x)[0]))

// e1000.c
void            e1000_init(uint32 *);
void            e1000_intr(void);
int             e1000_transmit(struct mbuf*);

// net.c
void            net_rx(struct mbuf*);
void            net_tx_udp(struct mbuf*, uint32, uint16, uint16);
void            net_rx_tcp(struct mbuf*, uint16, struct ip*);

// sysnet.c
void            sockinit(void);
int             sockalloc(struct file **, uint32, uint16, uint16);
void            sockclose(struct sock *);
int             sockread(struct sock *, uint64, int);
int             sockwrite(struct sock *, uint64, int);
void            sockrecvudp(struct mbuf*, uint32, uint16, uint16);