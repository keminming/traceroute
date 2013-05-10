/*
	KE Wang
	822002009
*/
#pragma once

#define RECV_BUF_MAX 1024
#pragma comment (lib, "ws2_32.lib") 


class Socket
{
private:
	SOCKET sock;
	char* recv_buf;
	int recv_buf_size;
	int total_recv_count;
public:
	Socket():recv_buf_size(RECV_BUF_MAX),total_recv_count(0){startup();recv_buf = new char[RECV_BUF_MAX];}
	const char* get_recv_buf(){return recv_buf;}
	static int startup();
    bool ICMP_sock_create();
	bool DNS_sock_create();
    bool sock_send(char* send_buf,int count,char* address,char* port);
    bool sock_recv();
	bool sock_set_ttl(int ttl);
	SOCKET* get_sock(){return &sock;}
	~Socket(){delete recv_buf;}
};

