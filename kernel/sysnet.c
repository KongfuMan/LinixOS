#include "param.h"
#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "fs.h"
#include "buf.h"
#include "net.h"
#include "defs.h"
#include "file.h"
#include "stat.h"

struct sock {
    struct sock *next; // the next socket in the list
    uint32 raddr;      // the remote IPv4 address
    uint16 lport;      // the local UDP port number
    uint16 rport;      // the remote UDP port number
    struct spinlock lock; // protects the rxq
    struct mbufq rxq;  // a queue of packets waiting to be received
};

void
sockinit(void){
}

int
sockalloc(struct file **f, uint32 raddr, uint16 lport, uint16 rport){
    return 0;
}

void
sockclose(struct sock *si){
}

int
sockread(struct sock *si, uint64 addr, int n){
    return 0;
}

int
sockwrite(struct sock *si, uint64 addr, int n){
    return 0;
}

// called by protocol handler layer to deliver UDP packets
void
sockrecvudp(struct mbuf *m, uint32 raddr, uint16 lport, uint16 rport){
}