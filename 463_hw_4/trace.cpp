#include "stdafx.h"
#include "trace.h"

extern queue<int> number_hops;
extern queue<int> retrans;
extern queue<string> ips;
extern std::set<string> results;
extern queue<pair<int,string>> hop_host;
extern int total_pkt;
extern queue<int> errors;
extern queue<int> delays;


void Trace::run_trace(char* host)
{	
	/*start*/
	QueryPerformanceCounter(&start);
	hostent* remote_host = gethostbyname(host);
	if(remote_host == NULL)
		return;
	struct in_addr addr;
	addr.s_addr = *(unsigned long*)remote_host->h_addr_list[0];
	std::cout<<"Tracerouting to "<<inet_ntoa(addr)<<"...\n";
	string name;
	ICMP_info info;
	pair<int,string> dns_result;
	int dns_id = 0;
	hop* h;
	int conunt = 1;

	for(int i = 1; i <= 30; i++)
	{	
		/*send icmp request*/
		icmp.send_ICMP_request(inet_ntoa(addr),i);
		total_pkt++;
		hops[i -1].ttl = i;
		hops[i -1].probes = 1;
		event e;
		QueryPerformanceCounter(&hops[i -1].t1);
		e.type = EVENT_ICMP;
		e.ttl = i;
		e.due_time = 1000*((double)hops[i - 1].t1.QuadPart - (double)start.QuadPart)/(double)feq.QuadPart + hops[i -1].RTO;
		event_queue.push(e);
	}

	while(!event_queue.empty())
	{
		event e = event_queue.top();
		event_queue.pop();
		int ret;
		QueryPerformanceCounter(&end);
		int timeout = e.due_time - 1000*((double)end.QuadPart - (double)start.QuadPart)/(double)feq.QuadPart;
		if(timeout > 0)
			nEvent = WaitForMultipleObjects(2, handle_set.data(), FALSE, timeout/100);
		else
			nEvent = WaitForMultipleObjects(2, handle_set.data(), FALSE, 0);
		switch(nEvent)
		{
		case WAIT_OBJECT_0:
			/*recv icmp response*/
			ret = icmp.recv_ICMP_response(&info);
			ips.push(info.ip);
	
			if(ret == -1){
				break;
			}
			if(ret == -2)
			{
				break;
			}
			else
			{
				/*send dns request*/
				dns_id = dns.send_DNS_request(info.ip);
				if(dns_id == -1)
					break;
				if(info.ttl < 1)
					break;
				h = &hops[info.ttl - 1];
				DNS_map.insert(make_pair(dns_id,h));
			}
			if(info.ttl < 1)
				break;
			hops[info.ttl - 1].ip = info.ip;	
			hops[info.ttl- 1].t2 = end;
			hops[info.ttl- 1].RTO = ((double)hops[info.ttl- 1].t2.QuadPart - (double)hops[info.ttl- 1].t1.QuadPart)*1000/(double)feq.QuadPart;

			/*make dns event*/
			e.due_time = 1000*((double)end.QuadPart - (double)start.QuadPart)/(double)feq.QuadPart + 5000;/*ms*/
			e.type = EVENT_DNS;
			event_queue.push(e);
			break;
		case WAIT_OBJECT_0 + 1:
			/*recv dns response*/
			dns_result = dns.get_DNS_answer();
			if(dns_result.first == -1)
			{	
				break;
			}

			h = DNS_map[dns_result.first];
			if(h == NULL)
				break;
		   /* if(results.find(dns_result.second) != results.end())
				break;*/

			results.insert(dns_result.second);
			h->result = dns_result.second;
			std::cout<<conunt<<" "<<dns_result.second<<" ("<<h->ip<<") "<<1000*((double)h->t2.QuadPart - (double)h->t1.QuadPart)/(double)feq.QuadPart<<"ms ("<<h->probes<<")\n";
			conunt++;
			break;
		case WAIT_TIMEOUT:
			if(e.type == EVENT_ICMP)
			{
				h = &hops[e.ttl-1];
				if(h->probes > 3){
					printf("wait time out.\n");
					break;
				}

				icmp.send_ICMP_request(inet_ntoa(addr),conunt);
				total_pkt++;
				e.type = EVENT_ICMP;
				h->probes++;
				int i,j;
				int RTO;
				while(1)
				{
					i = e.ttl - 2;
					j = e.ttl;
					if(i >= 0 && j <= e.ttl - 1)
					{
						if(hops[i].RTO != INITIAL_RTO && hops[j].RTO != INITIAL_RTO)
						{	
							RTO = (hops[e.ttl - 2].RTO + hops[e.ttl].RTO);
							break;
						}
						else if(hops[i].RTO == INITIAL_RTO)
							i++;
						else if(hops[j].RTO == INITIAL_RTO)
							j++;
					}
					else if(i < 0 && j <= e.ttl -1)
					{
						if(hops[j].RTO != INITIAL_RTO)
						{	
							RTO = 2* hops[j].RTO;
						}
						else
						{
							RTO = INITIAL_RTO;
						}
						break;
					}
					else
					{
						RTO = INITIAL_RTO;
						break;
					}					
				}

				e.due_time = 1000*((double)end.QuadPart - (double)start.QuadPart)/(double)feq.QuadPart + RTO;
				event_queue.push(e);
			}

		case WAIT_FAILED:
			break;
		}
	}
FINAL:
	QueryPerformanceCounter(&end);
	std::cout<<"\n"<<"Total execution time: "<<1000*((double)end.QuadPart - (double)start.QuadPart)/(double)feq.QuadPart<<" ms\n";
	getch();
	hop_host.push(make_pair(conunt,host));
	number_hops.push(conunt);
	int hop_sum = 0;
	for(int i=0;i<hops.size();i++)
		hop_sum += hops[i].probes;
	retrans.push(hop_sum);
}

