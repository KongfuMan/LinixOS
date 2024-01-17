// tcp header
typedef struct {
    uint16  src_port;       //source port
    uint16  dst_port;       //destination port
    uint32  seq_no;         //sequence number
    uint32  ack_no; 
    uint8   rsv1:4,      // 4 reserved bits
            len:4;       // header length
    uint8   fin:1,       // FIN
            syn:1,       // SYN
            rst:1,       // RST
            psh:1,       // PSH
            ack:1,       // ACK
            urg:1,       // URG
            ece:1,       // ECN Echo
            cwr:1;       // Congestion window reduced
    uint16  window;      // recv window
    uint16  checksum;
    uint16  urgptr;      // urgent data pointer
} tcphdr_t;  //__attribute__ ((packed))

// #define CWR (1<<7)
// #define ECE (1<<6)
// #define URG (1<<5)
// #define ACK (1<<4)
// #define PSH (1<<3)
// #define RST (1<<2)
// #define SYN (1<<1)
// #define FIN (1<<0)