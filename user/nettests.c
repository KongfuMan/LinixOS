#include "kernel/types.h"
#include "kernel/net.h"
#include "user.h"

static const uint32 dst_ip = MAKE_IP_ADDR(192, 168, 1, 243);
static const uint16 dst_port = 1234;

int tcp_client(uint16 dst_port){
    int sockfd;
    // uint32 dst_ip = MAKE_IP_ADDR(192, 168, 1, 243);
    int domain = 0, type = 0, protocol = IPPROTO_TCP;
    if ((sockfd = socket(domain, type, protocol)) < 0){
        fprintf(2, "create tcp sock failed. \n");
        exit(1);
    }
    if (bind(sockfd, 0, TCPPORT) < 0){
        fprintf(2, "bind tcp sock failed. \n");
        exit(1);
    }
    fprintf(1, "bind tcp sock success. \n");

    // you can send a TCP packet to any Internet address
    // by using a different dst.
    if(connect(sockfd, dst_ip, dst_port) < 0){
    }
    while(1);

    char *obuf = "a message from xv6!";
}

void udp_client(uint16 dst_port){
    int sockfd;
    int domain = 0, type = 0, protocol = IPPROTO_UDP;
    if ((sockfd = socket(domain, type, protocol)) < 0){
        fprintf(2, "create udp sock failed. \n");
        exit(1);
    }
    if (bind(sockfd, 0, UDPPORT) < 0){
        fprintf(2, "bind udp sock failed. \n");
        exit(1);
    }
    fprintf(1, "bind udp sock success. \n");
    
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
    fprintf(1, "run socket client.\n");
    tcp_client(dst_port);
    while(1);
    return 0;
}