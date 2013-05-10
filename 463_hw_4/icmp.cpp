#include "stdafx.h"
#include "icmp.h"

using namespace std;
/*
* ======================================================================
* ip_checksum: compute ICMP header checksum.
*
* Returns the checksum. No errors possible.
*
* ======================================================================
*/
extern queue<int> errors;

unsigned short ip_checksum (unsigned short *buffer, int size)
{
	unsigned long cksum = 0;
	/* sum all the words together, adding the final byte if size is odd */
	while (size > 1)
	{
		cksum += *buffer++;
		size -= sizeof (unsigned short);
	}
	if (size)
	cksum += *(unsigned char *) buffer;
	/* add carry bits to lower u_short word */
	cksum = (cksum >> 16) + (cksum & 0xffff);
	/* return a bitwise complement of the resulting mishmash */
	return (unsigned short) (~cksum);
}

int ICMP::send_ICMP_request(char* addr,int TTL)
{
	int pkt_size = ICMP_HDR_SIZE;
	unique_ptr<char> send_buf(new char[pkt_size]);
	memset(send_buf.get(),0,pkt_size);

	ICMPHeader *orig_icmp_hdr = (ICMPHeader *) send_buf.get();

	make_ICMP_pkt(orig_icmp_hdr,TTL);

    if(s.sock_set_ttl(TTL) == -1)
	{
		return -1;
	}

	if(!s.sock_send(send_buf.get(),pkt_size, addr,"0"))
	{
		printf("Send ICMP request fail.\n");
		return -1;
	}
	return 0;
}

void ICMP::make_ICMP_pkt(ICMPHeader* icmp,int seq)
{
	// set up the echo request
	// no need to flip the byte order since fields are 1 byte each
	icmp->type = ICMP_ECHO_REQUEST;
	icmp->code = 0;
	// set up ID/SEQ fields as needed
	icmp->id = htons((unsigned short)GetCurrentProcessId());
	icmp->seq = htons(seq);
	/* calculate the checksum */
	int packet_size = sizeof(ICMPHeader); // 8 bytes
	icmp->checksum = ip_checksum ((unsigned short *) icmp, sizeof(ICMPHeader));
}



int ICMP::recv_ICMP_response(ICMP_info* info)
{
	if(!s.sock_recv())
	{
		printf("No ICMP response.\n");
		return -1;
	}
	IPHeader *router_ip_hdr = (IPHeader *)s.get_recv_buf();
	ICMPHeader *router_icmp_hdr = (ICMPHeader *) (router_ip_hdr + 1);
	IPHeader *orig_ip_hdr = (IPHeader *) (router_icmp_hdr + 1);
	ICMPHeader *orig_icmp_hdr = (ICMPHeader *) (orig_ip_hdr + 1);
	
	errors.push(router_icmp_hdr->type);

	// check to see if this is TTL_expired; make sure packet size >= 56 bytes
	if (router_icmp_hdr->type == ICMP_TTL_EXPIRE && router_icmp_hdr->code == 0)
	{
		if (orig_ip_hdr->proto == IPPROTO_ICMP)
		{
			// check if process ID matches
			if (ntohs(orig_icmp_hdr->id) == GetCurrentProcessId())
			{
				// take router_ip_hdr->source_ip and
				unsigned long ip = router_ip_hdr->source_ip;
				// initiate a DNS lookup
				in_addr addr;
				addr.S_un.S_addr = ip;
				info->ttl = orig_ip_hdr->ttl;
				info->ip = inet_ntoa(addr);
				return 0;
			}
		}
	}
	else if(router_icmp_hdr->type == ICMP_ECHO_REPLY)
	{
		return -2;
	}
	
	printf("Icmp error:  type[%d] code[%d]\n",router_icmp_hdr->type,router_icmp_hdr->code);
	return -1;
}
