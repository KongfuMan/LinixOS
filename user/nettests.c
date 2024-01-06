#include "kernel/types.h"
#include "kernel/net.h"
#include "user.h"

int tcp_client(uint16 dport, uint16 sport){
    int sockfd;
    int domain = 0, type = 0, protocol = IPPROTO_TCP;
    if ((sockfd = socket(domain, type, protocol)) < 0){
        return -1;
    }
    fprintf(1, "client sock fd: %d. \n", sockfd);
    uint32 dstip;
    // 10.0.2.2, which qemu remaps to the external host,
    // i.e. the machine you're running qemu on.
    dstip = (192 << 24) | (168 << 16) | (1 << 8) | (243 << 0);
    // you can send a TCP packet to any Internet address
    // by using a different dst.
    if(connect(sockfd, dstip, sport, dport) < 0){
    }
    while(1);

    char *obuf = "a message from xv6!";
}

void udp_client(uint16 dest_port, uint16 src_port){
    int sockfd;
    uint32 dstip;
    int domain = 0, type = 0, protocol = IPPROTO_UDP;
    if ((sockfd = socket(domain, type, protocol)) < 0){
        fprintf(2, "create udp sock failed. \n");
        exit(1);
    }

    if (bind(sockfd, 0, src_port) < 0){
        fprintf(2, "bind udp sock failed. \n");
        exit(1);
    }

    fprintf(1, "binded udp sock success. sockfd: %d. \n", sockfd);
    
    // 10.0.2.2, which qemu remaps to the external host,
    // i.e. the machine you're running qemu on.
    dstip = (192 << 24) | (168 << 16) | (1 << 8) | (243 << 0);
    
    char *obuf = "Liang: a message from xv6!";
    // send udp message
    if(write(sockfd, obuf, strlen(obuf)) < 0){
        fprintf(2, "ping: send() failed\n");
        exit(1);
    }

    char ibuf[128];
    while(1){
        memset(ibuf, '\0', 128);
        int cc = read(sockfd, ibuf, sizeof(ibuf)-1);
        if(cc < 0){
            fprintf(2, "ping: recv() failed\n");
            exit(1);
        }
        fprintf(1, "received udp payload: %s. \n", ibuf);
    }
}

int main(){
    uint16 dport = 1234, sport = 2000;
    fprintf(1, "run socket client.\n");
    udp_client(dport, sport);
    while(1);
    return 0;
}