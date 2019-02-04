#include <iostream>
#include <string>
#include <deque>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include "Packet.h"
#include "socket_func.h"
#include <cstdio>
#include <thread>
#include <pthread.h>
#include <ctime>
#include <set>
#include <chrono>  

using std::string;
using std::deque;
using std::thread;
using std::set;
class Client
{
public:
	Client(string client_port,string host_name,string server_port,string file_name);
	void executeHandShake();
	void getFile();
	void closeConnection();
	~Client();
private:
	string sPort;
	string cPort;
	string hName;
	string fName;//filename
	int listenFd;
	int sendFd;
	struct addrinfo *target_ai;
	struct sockaddr_storage their_addr;//just for receiving
	int seqNum;
	int ackNum;
	set<int> seq_set;

	Packet* fnPkt;
	thread* fnThread;
};

Client::Client(string client_port,string host_name,string server_port,string file_name){
	srand(time(NULL));
	cPort=client_port;
	sPort=server_port;
	hName=host_name;
	fName=file_name;
	seqNum=getInitialSeq();
	// seqNum=3;
	
}

void Client::executeHandShake(){
	sendFd=getSendFd(hName.c_str(),sPort.c_str(),&target_ai);
	listenFd=getListenFd(cPort.c_str());
	
	Packet* hsPkt=new Packet(0,seqNum,0,1,0,(char *)"handShake");
	hsPkt->send(sendFd,target_ai);
	seq_set.insert(hsPkt->SEQ);
	thread* t=new thread(checkTimeOut,hsPkt,sendFd,target_ai,&seq_set);
	Packet rPkt=Packet();
	while(1){
		rPkt.receive(listenFd,&their_addr);
        if(rPkt.ACK==nextSeq(seqNum,1)&&rPkt.SYN_flag==1){
        	seq_set.erase(hsPkt->SEQ);
        	t->join();
        	break;
        }
	}

	seqNum=nextSeq(seqNum,1);
	ackNum=nextAck(rPkt.SEQ,1);
	fnPkt=new Packet(ackNum,seqNum,1,0,0,fName.c_str());
	fnPkt->send(sendFd,target_ai);
	seq_set.insert(fnPkt->SEQ);
	fnThread=new thread(checkTimeOut,fnPkt,sendFd,target_ai,&seq_set);
}

void Client::getFile(){
	deque<Packet*> window=deque<Packet*>(5);
	for (int i=0;i<5;i++)
		window[i]=NULL;
	
	FILE * fPtr = fopen("received.data", "w");
    if(fPtr==NULL){printf("Fopen error\n");exit(1);}

    bool delete_thread=0;
	bool eof=0;
	while(1){
		Packet* rPkt=new Packet();
		rPkt->receive(listenFd,&their_addr,0);

		//get FNPKT PACKET
		if(delete_thread==0&&rPkt->SEQ==fnPkt->ACK){
			seq_set.erase(fnPkt->SEQ);
			delete_thread=1;
		}

		bool validPkt=0;
		for (int i=0;i<5;i++){
			if(rPkt->SEQ==nextAck(ackNum,i*MAX_PAYLOAD_SIZE)&&window[i]==NULL){
				window[i]=rPkt;
				validPkt=1;
				printf("i: %d\n",i);
				break;
			}
		}
		//response
		if(validPkt==0){
			seqNum=nextSeq(seqNum,1);
			printf("*******%s\n", "Invalid Packet Not in the Window");
			printf("Cuurent ACK %d\n", ackNum);
			Packet(ackNum,seqNum,1,0,0,(char*)"").send(sendFd,target_ai);
			// exit(1);
		}
		else{
			while(window.front()!=NULL){
				ackNum=nextAck(window.front()->SEQ,MAX_PAYLOAD_SIZE);
				int len;
				memcpy(&len,window.front()->payload,4);
				fwrite(&window.front()->payload[4],sizeof(char),len,fPtr);
				if(len<(MAX_PAYLOAD_SIZE-4))
					eof=1;
				window.pop_front();
				window.push_back(NULL);
			}
			// ackNum=nextAck(ackNum,MAX_PAYLOAD_SIZE);
			seqNum=nextSeq(seqNum,1);
			Packet(ackNum,seqNum,1,0,0,(char*)"").send(sendFd,target_ai);
		}
		if(eof){
			fclose(fPtr);
			break;
		}
	}
}

void Client::closeConnection(){
	printf("Start closeConnection\n");
	seqNum=nextSeq(seqNum,1);
	Packet* fPkt=new Packet(ackNum,seqNum,1,0,1,(char*)"");
	fPkt->send(sendFd,target_ai);
	seq_set.insert(fPkt->SEQ);
	thread *t=new thread(checkTimeOut,fPkt,sendFd,target_ai,&seq_set);

	Packet rPkt=Packet();
	while(1){
		rPkt.receive(listenFd,&their_addr);
		if(rPkt.FIN_flag==1){
			seq_set.erase(fPkt->SEQ);
			if(t->joinable())
				t->join();
			seqNum=nextSeq(seqNum,1);
			ackNum=nextAck(rPkt.SEQ,1);
			Packet sPkt=Packet(ackNum,seqNum,1,0,0,(char*)"");
			sPkt.send(sendFd,target_ai);
			clock_t c=clock();
			while(1){
				if(diffclock(clock(),c)>=2*TIME_OUT)
					break;
			}
			break;
		}
	}
}

Client::~Client(){
	freeaddrinfo(target_ai);
    close(listenFd);
    close(sendFd);
}

