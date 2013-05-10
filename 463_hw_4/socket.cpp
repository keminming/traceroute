#include "stdafx.h"
#include "socket.h"
#include <ws2tcpip.h>

#define ERROR_FAIL 1

int Socket::startup()
{
	WORD Version;
	WSADATA Data;
	Version = MAKEWORD(2, 2);
	int ret  = WSAStartup(Version, &Data);
	if(ret != ERROR_SUCCESS)
	{
		printf("Winsock DLL not found.\n");
		return ERROR_FAIL;
	}

	if (LOBYTE(Data.wVersion) != 2 || HIBYTE(Data.wVersion) != 2 )
	{
		/* Tell the user that we could not find a usable WinSock DLL.*/
		printf("Server: The dll do not support the Winsock version %u.%u!\n", LOBYTE(Data.wVersion), HIBYTE(Data.wVersion));
		return ERROR_FAIL;
	}
	
	return ERROR_SUCCESS;
}

bool Socket::ICMP_sock_create()
{
	/* ready to create a socket */
	if((sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == INVALID_SOCKET)
	{
		printf("Fail to create a raw socket with error = %d.\n",WSAGetLastError());
		return false;//return ERROR_FAIL;
	}
	return true;
}

bool Socket::DNS_sock_create()
{
	if((sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) == INVALID_SOCKET)
	{
		printf("Fail to create socket with error = %d.\n",WSAGetLastError());
		return false;//return ERROR_FAIL;
	}

	bool option = true;
	if(setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char*)&option,sizeof(bool)) == SOCKET_ERROR)
	{
        printf("Setsockopt for SO_REUSEADDR failed with error: %u\n", WSAGetLastError());
		return false;
	}

	SOCKADDR_IN server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = 0;

	if(bind(sock,(const sockaddr*)&server,sizeof(sockaddr))
		== SOCKET_ERROR)
	{
		printf("Fail to connect to server with error = %d.\n",WSAGetLastError());
		return false;
	}

	return true;
}

bool Socket::sock_set_ttl(int ttl)
{
	if (setsockopt (sock, IPPROTO_IP, IP_TTL, (const char *) &ttl,  
		sizeof (ttl)) == SOCKET_ERROR) {
			printf ("setsockopt failed with %d\n", WSAGetLastError());
			return -1;
	}
	return 0;
}

bool Socket::sock_send(char* send_buf,int count,char* address,char* port)
{	
	SOCKADDR_IN server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(address);
	server.sin_port = htons(atoi(port));
	
	if(sendto(sock,send_buf,count,0, (SOCKADDR*)&server,sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		printf("Fail to send to server with error = %d.\n",WSAGetLastError());
		return false;
	}
	return true;
}


bool Socket::sock_recv()
{
	SOCKADDR_IN server;	
	memset(recv_buf,0,recv_buf_size);
	int recv_count = 0;
	fd_set fd;
	FD_ZERO(&fd);
	FD_SET(sock, &fd);
	int ret;
	timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	int count = 0;

	ret = select(0, &fd, NULL,NULL, &timeout);/*two use of the select, 1 poll sockets, 2 set block timeout*/
	if(ret > 0)  
	{
		int size = sizeof(SOCKADDR);
		recvfrom(sock,recv_buf,recv_buf_size - total_recv_count,0,(SOCKADDR*)&server, &size);
		return true;
	}
	else if(ret == 0)
	{
		printf("select timeout.\n");
		return false;
	}
	else if(ret == SOCKET_ERROR)
	{
		printf("Fail to select = %d.\n",WSAGetLastError());
		return false;
	}

	return false;
}

