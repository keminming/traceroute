#include "stdafx.h"
#include "icmp.h"
#include "dns.h"


enum event_type
{
	EVENT_ICMP,
	EVENT_DNS
};

class event
{
public:
	event_type type; /*0:ICMP 1:DNS*/
	int due_time;
	int ttl;/*external key*/
	event():due_time(0){}
};

#define INITIAL_RTO 500

class hop
{
public:
	LARGE_INTEGER t1;
	LARGE_INTEGER t2;
	double RTO;
	string result;
	int probes;
	int ttl;/*primary key*/
	string ip;
	bool finish;
	hop():RTO(INITIAL_RTO),probes(0),finish(false){}
};

class compare_time_seq
{
public:
  bool operator() (const event& e1, const event& e2) const
  {
      return e1.due_time > e2.due_time;
  }
};

class Trace
{
private:
    ICMP icmp;
	DNS dns;
	DWORD nEvent;
	vector<hop> hops;
	unordered_map<int,hop*> DNS_map; 
	priority_queue<event,vector<event>,compare_time_seq> event_queue;
	HANDLE hICMPEvent;
	HANDLE hDNSEvent;
	vector<HANDLE> handle_set;
	LARGE_INTEGER feq;
	LARGE_INTEGER start;
	LARGE_INTEGER end;
public:
	Trace()
	{
		hops.resize(30);	
		hICMPEvent = CreateEvent(NULL,false,false,NULL);
		hDNSEvent =  CreateEvent(NULL,false,false,NULL); 
		handle_set.push_back(hICMPEvent); 
		handle_set.push_back(hDNSEvent);
		WSAEventSelect(*(icmp.get_sock()), hICMPEvent,FD_READ); 
		WSAEventSelect(*(dns.get_sock()), hDNSEvent,FD_READ);
		QueryPerformanceFrequency(&feq);
	}
	void run_trace(char* host);
};

