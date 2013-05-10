// 463_hw_4.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "trace.h"

bool stop = false;
queue<int> number_hops;
queue<int> retrans;
queue<string> ips;
std::set<string> results;
queue<pair<int,string>> hop_host;
int total_pkt = 0;
queue<int> errors;
queue<int> delays;

DWORD WINAPI stat(void* param)
{
	ofstream o1,o2,o3,o4,o5,o6,o7;
	o1.open("hops.csv");
	o2.open("hops_and_host.txt");
	o3.open("ips.txt");
	o4.open("delays.csv");
	o5.open("retx.csv");
	o6.open("pks.csv");
	o7.open("error.csv");
	int ip_count = 0;

	while(!stop)
	{
		while(!number_hops.empty())
		{
			o1<<number_hops.front()<<"\n";
			number_hops.pop();
		}

		while(!hop_host.empty())
		{
			o2<<hop_host.front().first<<" "<<hop_host.front().second<<"\n";
			hop_host.pop();
		}

		
		while(!ips.empty())
		{
			o3<<ip_count++<<" "<<ips.front()<<"\n";
			ips.pop();
		}

		while(!delays.empty())
		{
			o4<<delays.front()<<"\n";
			delays.pop();
		}
		
		while(!retrans.empty())
		{
			o5<<retrans.front()<<"\n";
			retrans.pop();
		}

		while(!errors.empty())
		{
			o7<<errors.front()<<"\n";
			errors.pop();
		}

		o6<<total_pkt<<"\n";

		Sleep(2000);
		o1.flush();
		o2.flush();
		o3.flush();
		o4.flush();
		o5.flush();
		o6.flush();
		o7.flush();
	}

	return 0;
}

//int _tmain(int argc, _TCHAR* argv[])
//{
//	HANDLE thread = CreateThread(NULL,0,stat,NULL,0,0);
//	Trace tr;
//	ifstream f("dns-in.txt");
//	ofstream of("trace.txt");
//	queue<string> hosts;
//	int start,end;
//	int count = 0;
//	while(!f.eof() && count < 1000)
//	{
//		string line;
//		getline(f,line);
//		start = GetTickCount();
//		tr.run_trace((char*)line.c_str());
//		end = GetTickCount();
//		delays.push(end - start);
//		count++;
//	}
//	stop = true;
//	WaitForSingleObject(thread,INFINITE);
//}
//
int _tmain(int argc, _TCHAR* argv[])
{

	Trace tr;

	tr.run_trace((char*)argv[1]);

}