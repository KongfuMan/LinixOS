/* below are APIs public to kernel. */

struct proc;
struct cpu;

// console.c
void consoleinit(void);
void consoleintr(char);
void consputc(char);

// uart.c
void uartinit(void);
void uart_putc(char c);
char uart_getc(void);

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

// vm.c
void kvminit(void);
void kvmmap(pagetable_t, uint64, uint64, uint64, int);
void kvminithart(void);
int mappages(pagetable_t, uint64, uint64, uint64, int);
void uvmunmap(pagetable_t, uint64, int, int);
pagetable_t uvmcreate(void);
void uvmfirst(pagetable_t, uchar *, uint);
int uvmcopy(pagetable_t, pagetable_t, uint64);
pte_t * walk(pagetable_t pgtable, uint64 va, int alloc);

//proc.c
void proc_mapstacks(pagetable_t);
void procinit(void);
int cpuid(void);
void userinit(void);
void scheduler(void);
struct cpu* current_cpu();
struct proc* current_proc(void);
void sleep(void*);
void sched(void);
pid_t fork(void);

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

// syscall.c
void syscall(void);

#define NELEM(x) (sizeof(x)/sizeof((x)[0]))
