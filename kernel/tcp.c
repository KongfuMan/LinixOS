#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "fs.h"
#include "buf.h"
#include "net.h"
#include "defs.h"
#include "memlayout.h"
#include "tcp.h"


/*
 * Compute TCP checksum
 * Parameter:
 * @words - pointer to IP payload, in network byte order
 * @byte_counts - number of bytes
 * @ip_src - IP source address, in network byte order
 * @ip_dst - IP destination address, in network byte order
 * Result:
 * TCP checksum, in host byte order
 */
static uint16
compute_checksum(uint16* words, uint16 byte_count,  uint32 ip_src, uint32 ip_dst) {
	uint32 sum = 0;
	uint16 rc;
	int i;
	uint16 last_byte = 0;
	/*
 	* First add all fields in the 12 byte pseudo-header:
 	* 4 byte bit source IP address
 	* 4 byte bit destination IP address
 	* 1 byte padding
 	* 1 byte IP protocol (6 = TCP)
 	* 2 bytes TCP segment length
 	* Instead of converting all fields to host byte order before adding them,
 	* we add up everything in network byte order and then convert the result
 	* This will give the same checksum (see RFC 1071), but will be faster
 	*/
	sum = 0x6*256 + +htons(byte_count);
	sum = sum + ((ip_src >> 16) & 0xFFFF) + (ip_src & 0xFFFF);
	sum = sum + ((ip_dst >> 16) & 0xFFFF) + (ip_dst & 0xFFFF);
	/*
 	* Sum up all other words
 	*/
	for (i = 0; i < byte_count / 2; i++) {
    	sum = sum + words[i];
	}
	/*
 	* If the number of bytes is odd, add left over byte << 8
 	*/
	if (1 == (byte_count % 2)) {
    	last_byte = ((uint8*) words)[byte_count - 1];
    	sum = sum + last_byte;
	}
	/*
 	* Repeatedly add carry to LSB until carry is zero
 	*/
	while (sum >> 16)
    	sum = (sum >> 16) + (sum & 0xFFFF);
	rc = sum;
	rc = htons(~rc);
	return rc;
}

extern uint32 local_ip;

// client initiate 3-way handshake to establish tcp connection
// 
void tcp_connect(uint32 dst_ip, uint16 src_port, uint16 dst_port){
    struct mbuf *m = mbufalloc(MBUF_DEFAULT_HEADROOM);
    if (!m)
        return;
    tcphdr_t *tcphdr;
    tcphdr = mbufpushhdr(m, *tcphdr);
    memset(tcphdr, 0, sizeof(*tcphdr));
    tcphdr->src_port = htons(src_port);
    tcphdr->dst_port = htons(dst_port);
    tcphdr->seq_no = htonl(12345);    // generate an random intial sequence number
    tcphdr->ack_no = htonl(0);
    tcphdr->len = m->len / 4;
    tcphdr->syn = 1;
    tcphdr->window = htons(8192);
    uint16 checksum = compute_checksum((uint16*)tcphdr, m->len,
                                        htonl(local_ip), htonl(dst_ip));
    tcphdr->checksum = htons(checksum);
    net_tx_tcp(m, dst_ip);
}

void tcp_recv(struct mbuf *m){

}

void tcp_send(struct mbuf *m);
