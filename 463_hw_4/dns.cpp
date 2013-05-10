/*
	KE Wang
	822002009
*/
#pragma comment (lib,"Iphlpapi.lib")
#include "stdafx.h"
#include "dns.h"

CRITICAL_SECTION statistic_lock;
statistic stat;

bool is_integer(string s)
{
	for(unsigned int i = 0; i < s.size(); i++)
		if(s[i] > '9' || s[i] < '0')
			return false;
	return true;
}

vector<string> split(string& s, char seperator)
{
	stringstream ss(s);
	string token;
	vector<string> result;
	while(!ss.eof())
	{
		std::getline(ss,token,seperator);
		result.push_back(token);
	}
	return result;
}

bool DNS::is_Ip_addr(string question)
{
	for(unsigned int i = 0; i < question.size(); i++)
	{
		if(('0' <= question[i] && question[i] <= '9') || (question[i] == '.'))
			continue;
		else return false;
	}
	return true;
}

bool DNS::is_valiad_IP(string ip)
{
	if(inet_addr(ip.c_str()) == INADDR_NONE)
		return false;
}


vector<string> DNS::getDNSServer(void)
{
	// MSDN sample code
	FIXED_INFO *FixedInfo;
	ULONG    ulOutBufLen;
	DWORD    dwRetVal;
	IP_ADDR_STRING * pIPAddr;
	vector<string> hosts;

	ulOutBufLen = sizeof(FIXED_INFO);
	FixedInfo = (FIXED_INFO *) GlobalAlloc( GPTR, sizeof( FIXED_INFO ) );
	ulOutBufLen = sizeof( FIXED_INFO );

	if(ERROR_BUFFER_OVERFLOW == GetNetworkParams(FixedInfo, &ulOutBufLen)) {
		GlobalFree( FixedInfo );
		FixedInfo = (FIXED_INFO *)GlobalAlloc( GPTR, ulOutBufLen );
	}

	if ( dwRetVal = GetNetworkParams( FixedInfo, &ulOutBufLen ) ) {
		printf( "Call to GetNetworkParams failed. Return Value: %08x\n", dwRetVal );
		return hosts;
	}
	else {
		//printf( "Host Name: %s\n", FixedInfo->HostName );
		//printf( "Domain Name: %s\n", FixedInfo->DomainName );

		//printf( "DNS Servers:\n" );
		//printf( "\t%s\n", FixedInfo->DnsServerList.IpAddress.String);
	    
		
		hosts.push_back(FixedInfo->DnsServerList.IpAddress.String);
		pIPAddr = FixedInfo->DnsServerList.Next;
		while ( pIPAddr ) {
			hosts.push_back(pIPAddr ->IpAddress.String);
			//printf( "\t%s\n", pIPAddr ->IpAddress.String);
			pIPAddr = pIPAddr ->Next;
		}	
		return hosts;
	}

	GlobalFree (FixedInfo);
}


void DNS::makeDNSquestion(char* question, vector<string>& tokens)
{
	char* pos = question;
	for(auto e : tokens)
	{
		*pos++ = e.size();
		memcpy(pos,e.c_str(),e.size());
		pos += e.size();
	}
	*pos = '\0';
}

string make_domain(const char* buf, int offset)
{
	unsigned char* start = (unsigned char*)buf + offset;
    unsigned char* current  = start;
	string ret;
	while(*current != 0)
	{
		if((*current & 0xc0) == 0xc0)
		{
			ret += make_domain(buf, *(current+1));
			return ret;
		}
		else
		{
			int len = *current;	
			for(int i=1; i <= len;i++)
			{
				ret += *(current + i);
			}

			ret += '.';
			current += (len + 1);
		}
	}
	return ret;
}

string DNS::make_result_string(const char* buf, int offset)
{
    fixedDNSheader * fdh = (fixedDNSheader*) buf;
	unsigned short Id = ntohs(fdh->ID);
	unsigned short flags = ntohs(fdh->flags);
	unsigned short answer_count = ntohs(fdh->answers);
	unsigned short authority_count = ntohs(fdh->name_server_records);
	unsigned short addition_count = ntohs(fdh->add_records);

	if((flags & RCODE_MASK) == DNS_SERVERFAIL)
	{
		return "Authoritative DNS server not found\n";
	}
	else if((flags & RCODE_MASK) == DNS_ERROR)
	{
		return "";
	}
	else if((flags & RCODE_MASK) != DNS_OK)
	{
		return "Other errors.\n";
	}

	unsigned char* current = (unsigned char*)buf + offset;
	int offset_data = 0;
	while(*(current + offset_data) != '\0')
		offset_data++;
	if(offset_data %2 != 0)
		offset_data++;
	offset_data += sizeof(fixedRR) + 2;

	return make_domain(buf,offset_data + offset);
}

int DNS::send_DNS_request(string hostname)
{
	unsigned short query_type;
	if(is_Ip_addr(hostname))
	{
		if(is_valiad_IP(hostname))
		{	
			query_type = DNS_PTR;
		}
		else
		{
			printf("Invalid IP address\n");
			return -1;
		}	
	}
	else
	{
		query_type = DNS_A;
	}

	unique_ptr<char> buf(new char[1024]);
		vector<string> tokens;
	tokens = split(hostname,'.');
	if(query_type == DNS_PTR)
	{
		vector<string> tmp;
		for(int i = tokens.size() - 1; i>=0; i--)
			tmp.push_back(tokens[i]);
		tmp.push_back("in-addr");
		tmp.push_back("arpa");
		tokens.clear();
		tokens = tmp;
	}
	int pkt_size = 0;	
	int question_size = 0;
	for(unsigned int i = 0; i < tokens.size(); i++)
		question_size += tokens[i].size() + 1;
	question_size += 1 + sizeof(queryHeader);
	pkt_size += sizeof(fixedDNSheader) + question_size;
		
	fixedDNSheader* dns_header = (fixedDNSheader*) buf.get();
	queryHeader* query_header = (queryHeader*)(buf.get() + pkt_size - sizeof(queryHeader));

	dns_header->ID = htons(++seq_num);
	dns_header->flags = htons(DNS_STDQUERY | DNS_RD);
	dns_header->questions = htons(1);
	dns_header->add_records = 0;
	dns_header->answers = 0;
	dns_header->name_server_records = 0;

	query_header->dns_class = htons(DNS_INET);
	query_header->type = htons(query_type);

	makeDNSquestion((char*)dns_header + sizeof(fixedDNSheader), tokens);

	string server_ip = server_ips[rand()%server_ips.size()];
	if(!s.sock_send(buf.get(),pkt_size,(char*)server_ip.c_str(),"53"))
	{	
		printf("socket send error.\n");
		return -1;
	}

	return seq_num;
}

pair<int,string> DNS::get_DNS_answer()
{
	if(!s.sock_recv())
		return make_pair(-1,"");
	
	fixedDNSheader * fdh = (fixedDNSheader*) s.get_recv_buf();
	unsigned short Id = ntohs(fdh->ID);
	unsigned short flags = ntohs(fdh->flags);
	unsigned short answer_count = ntohs(fdh->answers);
	unsigned short authority_count = ntohs(fdh->name_server_records);
	unsigned short addition_count = ntohs(fdh->add_records);

	if((flags & RCODE_MASK) == DNS_SERVERFAIL)
	{
		return make_pair(Id,"Authoritative DNS server not found");
	}
	else if((flags & RCODE_MASK) == DNS_ERROR)
	{
		return make_pair(Id,"<no DNS entry>");
	}
	else if((flags & RCODE_MASK) != DNS_OK)
	{
		return make_pair(Id,"Other errors");
	}

	unsigned char* current = (unsigned char*)s.get_recv_buf() + sizeof(fixedDNSheader);
	int offset_data = 0;
	
	/*parse query*/
	while(*(current + offset_data) != '\0')
	{
		if((*current & 0xc0) == 0xc0)
		{	
			offset_data++;
			break;
		}
		offset_data++;
	}
		
	offset_data++;
	offset_data += sizeof(queryHeader);
	
	/*parse name*/
	while(*(current + offset_data) != '\0')
	{
		if((*(current + offset_data) & 0xc0) == 0xc0)
		{	
			offset_data++;
			break;
		}
		offset_data++;
	}
	offset_data++;
	offset_data += sizeof(fixedRR) + 2;

	return make_pair(Id,make_domain(s.get_recv_buf(),sizeof(fixedDNSheader) + offset_data));
}


class parameter
{
public:
	queue<string>* questions;
	queue<string>* answers;
	CRITICAL_SECTION* cs;
};


void init_stat()
{
	stat.local_dns_timeout = 0;
	stat.no_auth_server = 0;
	stat.no_dns_record = 0;
	stat.sucess = 0;
	stat.total_queries = 0;
}

