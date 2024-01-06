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
    uint16 family;          // protocol family
    uint16 protocol;        // transport layer protocol
    uint16 type;            // communication type
    uint32 src_ip;          // the source IPv4 address
    uint32 dst_ip;         // the destination IPv4 address
    uint16 src_port;        // the source port
    uint16 dst_port;       // the destination port
    struct spinlock lock;   // protects the rxq
    struct mbufq rxq;       // a queue of packets waiting to be received

    // TCP specific fields
    uint32 seq;             // sequence number
    uint32 ack;
    struct sock *next;      // the next socket in the list
};

static struct spinlock socklock;
static struct sock *sockets; // list of sockets from all procs

void
sockinit(void){
    initlock(&socklock, "sockets");
}

// create a socket instance and wrap in file struct and connect with the dest host
int
sockalloc(struct file **f, uint16 family, uint16 type, uint16 protocol){
    struct sock *sock = (struct sock *)kalloc();
    sock->family = family;
    sock->type = type;
    sock->protocol = protocol;
    initlock(&sock->lock, "sock");

    if((*f = filealloc()) == 0){
        return -1;
    }

    mbufq_init(&sock->rxq);

    (*f)->type = FD_SOCK;
    (*f)->readable = 1;
    (*f)->writable = 1;
    (*f)->sock = sock;

    // prepend to list of sockets
    acquire(&socklock);
    sock->next = sockets;
    sockets = sock;
    release(&socklock);

    return 0;
}

void
sockclose(struct sock *si){
}

int
sockread(struct sock *si, uint64 dst_uva, int n){
    struct mbuf *m;
    int len = 0;
    acquire(&si->lock);
    while(mbufq_empty(&si->rxq)){
        sleep(&si->rxq, &si->lock);
    }

    m = mbufq_pophead(&si->rxq);
    release(&si->lock);

    len = min(m->len, n);
    // copy out to dst_uva
    if (copyout(current_proc()->pagetable, dst_uva, m->head, len) < 0){
        mbuffree(m);
        return -1;
    }

    mbuffree(m);
    return len;
}

int
sockwrite(struct sock *si, uint64 src_uva, int n){
    struct proc *p = current_proc();
    struct mbuf *m;

    m = mbufalloc(MBUF_DEFAULT_HEADROOM);
    if (!m)
        return -1;

    if (copyin(p->pagetable, mbufput(m, n), src_uva, n) == -1) {
        mbuffree(m);
        return -1;
    }

    // TODO: hard code for test
    // dst_ip:192.168.1.243, dst_port:1234
    uint32 dst_ip = (192 << 24) | (168 << 16) | (1 << 8) | (243 << 0);
    uint16 dst_port = 1234;

    si->dst_ip = dst_ip;
    si->dst_port = dst_port;

    if (si->protocol == IPPROTO_UDP){
        net_tx_udp(m, si->dst_ip, si->src_port, si->dst_port);
    }

    return n;
}

int
sockbind(struct sock *sock, uint32 src_ip, uint16 src_port){
    // must check src_port not in use.
    struct sock *pos;

    pos = sockets;
    while (pos) {
        if (pos->src_ip == src_ip && pos->src_port == src_port){
            // bind failed, ip/port already in use.
            return -1;
        }
        pos = pos->next;
    }
    sock->src_ip = src_ip;
    sock->src_port = src_port;

    return 0;
}

void
sockrecvudp(struct mbuf *m, uint32 dst_ip, uint16 src_port, uint16 dst_port){
    int i;
    struct file *f;
    struct sock *si;

    acquire(&socklock);
    si = sockets;
    while (si){
        if (si->dst_ip == dst_ip && si->src_port == src_port &&
            si->dst_port == dst_port){
                goto found;
                break;
            }
        si = si->next;
    }

    release(&socklock);
    mbuffree(m);
    return;

found:
    release(&socklock);
    acquire(&si->lock);
    mbufq_pushtail(&si->rxq, m);
    release(&si->lock);

    // wakeup consumer of si->rxq
    wakeup(&si->rxq);
}

int
sockconn(int sockfd, uint32 dest_ip, uint16 src_port, uint16 dest_port){
    if (sockfd < 0 || sockfd > NOFILE - 1){
        return -1;
    }

    struct proc *p;
    struct file *f;
    p = current_proc();
    f = p->ofile[sockfd];
    if (f->type != FD_SOCK || f->sock == 0){
        return -1;
    }

    f->sock->dst_ip = dest_ip;
    f->sock->src_port = src_port;
    f->sock->dst_port = dest_port;
    mbufq_init(&f->sock->rxq);

    if (f->sock->protocol == 0){
        // tcp
        tcp_connect(f->sock->dst_ip, f->sock->src_port, f->sock->dst_port);
    }

    return 0;
}