#pragma once

#include "socket.h"
#include "dns.h"

#define IP_HDR_SIZE 20 /* RFC 791 */
#define ICMP_HDR_SIZE 8 /* RFC 792 */
/* max payload size of an ICMP message originated in the program */
#define MAX_SIZE 65200
/* max size of an IP datagram */
#define MAX_ICMP_SIZE (MAX_SIZE + ICMP_HDR_SIZE)
/* the returned ICMP message will most likely include only 8 bytes



/* longer replies (e.g., 68 bytes) are possible */
#define MAX_REPLY_SIZE (IP_HDR_SIZE + ICMP_HDR_SIZE + MAX_ICMP_SIZE)
/* ICMP packet types */
#define ICMP_ECHO_REPLY 0
#define ICMP_DEST_UNREACH 3
#define ICMP_TTL_EXPIRE 11
#define ICMP_ECHO_REQUEST 8

/* remember the current packing state */
#pragma pack (push, traceroute)
#pragma pack (1)
/* define the IP header (20 bytes) */

class IPHeader {
public:
unsigned char h_len:4; /* 4 bits: length of the header in dwords */
unsigned char version:4; /* 4 bits: version of IP, i.e., 4 */
unsigned char tos; /* type of service (TOS), ignore */
unsigned short len; /* length of packet */
unsigned short ident; /* unique identifier */
unsigned short flags; /* flags together with fragment offset - 16 bits */
unsigned char ttl; /* time to live */
unsigned char proto; /* protocol number (6=TCP, 17=UDP, etc.) */
unsigned short checksum; /* IP header checksum */
unsigned long source_ip;
unsigned long dest_ip;
};


/* define the ICMP header (8 bytes) */
class ICMPHeader{
public:
unsigned char type; /* ICMP packet type */
unsigned char code; /* type subcode */
unsigned short checksum; /* checksum of the ICMP */
unsigned short id; /* application-specific ID */
unsigned short seq; /* application-specific sequence */
};
/* now restore the previous packing state */
#pragma pack (pop, traceroute)

struct ICMP_info
{
	string ip;
	int ttl;
};

class ICMP
{
public:
	ICMP(){s.ICMP_sock_create();}
	int send_ICMP_request(char* addr,int TTL);
    int recv_ICMP_response(ICMP_info* info);
	SOCKET* get_sock(){return s.get_sock();}
private:
	Socket s;
	void make_ICMP_pkt(ICMPHeader* icmp,int seq);
};




