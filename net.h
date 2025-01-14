#ifndef NET_H
#define NET_H

#include "mmap.h"
#include <linux/if_ether.h>
#define ETH_ALEN 6
#define VRFM_VERSION 1
//#define PROT_NUMBER 0x8915 maybe in the future use RoCE V1 (or V2)
#define PROT_NUMBER 0x612
enum vrfm_packet_type
{
    VRFM_MEM_SEND,
    VRFM_DUMP_ALL,
    VRFM_RESET,
    VRFM_EVENT
};

struct rfm_header
{
    u_int8_t version:4;
    enum vrfm_packet_type cmd:4;
    u_int32_t offset;
    u_int16_t size;
    u_int32_t crc;
    u_int16_t seq;
}__attribute__((packed));


#define CHUNK (ETH_DATA_LEN-sizeof(struct rfm_header))



struct net_rfm
{
    struct rfm_header header;
    char data[CHUNK];
}__attribute__((packed));


int sendpacket (unsigned int offset,unsigned int length);
int sendpackets (unsigned int offset,unsigned int lenght);
void net_shutdown(void);
int net_init(void);

#endif