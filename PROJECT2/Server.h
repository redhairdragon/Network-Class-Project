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
#include "fileIO.h"
#include <cstdio>
#include <ctime>
#include <thread>
#include <pthread.h>
#include <set>
using std::string;
using std::deque;
using std::cout;
using std::endl;
using std::thread;
using std::set;
class Server
{
public:
	Server(string server_port,string client_port);
	~Server();
	void executeHandShake();
	void sendFile();
	void closeConnection();
private:
	string fName;
	string sPort;
	string cPort;
	int listenFd;
	int sendFd;
	int seqNum;
	int ackNum;
	struct addrinfo *target_ai;
	struct sockaddr_storage their_addr;//just for receiving
	set<int> seq_set;
	thread hsThread;
	Packet* hsPkt;
	deque<Packet*> window=deque<Packet*>(5);
    deque<thread*> t=deque<thread*>(5);
};

Server::Server(string server_port,string client_port){
	srand(time(NULL));
	sPort=server_port;
	cPort=client_port;
	seqNum=getInitialSeq();
	// seqNum=1000;
	seq_set=set<int>();
}
void Server::executeHandShake(){
	listenFd=getListenFd(sPort.c_str());
	Packet rPkt=Packet();
    rPkt.receive(listenFd,&their_addr);
    while(1){
        if(rPkt.SYN_flag){
            break;
        }
    }
    char s[INET6_ADDRSTRLEN];
    const char* client_name;
    client_name=inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s);
    sendFd=getSendFd(client_name,cPort.c_str(),&target_ai);
    ackNum=nextAck(rPkt.SEQ,1);
    hsPkt=new Packet(ackNum,seqNum,1,1,0,(char *)"handShake");
    hsPkt->send(sendFd,target_ai);
    seq_set.insert(hsPkt->SEQ);
    hsThread=thread(checkTimeOut,hsPkt,sendFd,target_ai,&seq_set);
}
void Server::sendFile(){
	//get Filename Pkt
	Packet rPkt=Packet();
	seqNum=nextSeq(seqNum,1);
    while(1){
    	rPkt.receive(listenFd,&their_addr);
        if(rPkt.ACK==seqNum){
   			ackNum=nextAck(rPkt.SEQ,1);
        	fName=string(rPkt.payload);
        	seq_set.erase(hsPkt->SEQ);
            break;
        }
    }

    printf("Got Filename\n");
    
	for(int i=0;i<5;i++){
		window[i]=NULL;
		t[i]=NULL;
	}

	printf("Initial 5\n");
    //send intial 5
    bool eof=0;
	char buf[MAX_PAYLOAD_SIZE];
	fileIO fio(fName.c_str());
	for(int i =0;i<5;i++){
		Packet *pkt=new Packet();
		int chunk_size=fio.nextBatch(&pkt->payload[4],MAX_PAYLOAD_SIZE-4);
		memcpy(pkt->payload,&chunk_size,4);
		pkt->ACK=ackNum;
		pkt->ACK_flag=1;
		pkt->SEQ=seqNum;
		seqNum=nextSeq(seqNum,MAX_PAYLOAD_SIZE);
		window[i]=pkt;
		if(eof!=1){
			seq_set.insert(window[i]->SEQ);
			window[i]->send(sendFd,target_ai,0);
			t[i]=new thread(checkTimeOut,window[i],sendFd,target_ai,&seq_set);
		}
		if(chunk_size<MAX_PAYLOAD_SIZE-4){
			eof=1;
		}

	}
	//start receiving ack
	printf("Start Receiving\n");
	
	while(1){
		Packet rPkt=Packet();
		
		//收到结束
		rPkt.receive(listenFd,&their_addr);
		if(rPkt.FIN_flag==1){
			ackNum=nextAck(ackNum,1);
			seq_set.clear();
			break;
		}
		int stop_index=-1;
		printf("WINDOW:\n");
		for(int i=0;i<=5;i++){
			if(rPkt.ACK==nextAck(window[0]->SEQ,i*MAX_PAYLOAD_SIZE)){
				stop_index=i;
				printf("%d ", nextAck(window[0]->SEQ,i*MAX_PAYLOAD_SIZE));
				break;
			}
		}
		printf("\n");
		printf("REAL WINDOW: \n");
		for (int j=0;j<5;j++){
			printf("%d ",window[j]->SEQ);
		}
		printf("\n");

		ackNum=nextAck(rPkt.SEQ,1);
		if(stop_index!=-1){
			printf("stop_index= %d\n", stop_index);
			printf("******Valid Packet\n");
			for(int j=0;j<stop_index;j++){
				seq_set.erase(window.front()->SEQ);
				// if(t.front()->joinable())
				// 	t.front()->join();
				window.pop_front();
				t.pop_front();

				Packet *pkt=new Packet();
				int chunk_size=fio.nextBatch(&pkt->payload[4],MAX_PAYLOAD_SIZE-4);
				memcpy(pkt->payload,&chunk_size,4);
				pkt->ACK=ackNum;
				pkt->SEQ=seqNum;
				seqNum=nextSeq(seqNum,MAX_PAYLOAD_SIZE);
				pkt->ACK_flag=1;
				if(eof!=1){
					pkt->send(sendFd,target_ai,1,0);
				}
				window.push_back(pkt);
				if(seq_set.find(pkt->SEQ) == seq_set.end())
					seq_set.insert(pkt->SEQ);
				else{
					fprintf(stderr, "DUP THREAD\n");exit(1);
				}
				thread* tmp=new thread(checkTimeOut,pkt,sendFd,target_ai,&seq_set);
				t.push_back(tmp);
				if(chunk_size<MAX_PAYLOAD_SIZE-4){
					printf("********REACH EOF\n");
					eof=1;
				}
			}
		}
		else{//ACK not in window
			printf("*******Not in window\n");
			// for(int i=0;i<5;i++){
			// 	if(window[i]->SEQ==0){
			// 		printf("seqNum==0: ");
			// 		printf("ACK: %d\n",window[i]->ACK);
			// 		printf("ACK_flag: %d\n",window[i]->ACK_flag);
			// 		printf("payload len: %d\n",*(int*)(window[i]->payload));
			// 	}
			// }
			// exit(1);
		}
		
	}
}


void Server::closeConnection(){
	printf("Start Closing\n");
	while(!t.empty()){
		if(t.front()==NULL)
			break;
		if(t.front()->joinable())
			t.front()->join();
		t.pop_front();
	}

	printf("CLOSE 2\n");
	seqNum=nextSeq(seqNum,1);
	Packet* lastAck=new Packet(ackNum,seqNum,1,0,0,(char*)"");
	lastAck->send(sendFd,target_ai);
	delete lastAck;

	seqNum=nextSeq(seqNum,1);
	Packet *fPkt=new Packet(ackNum,seqNum,1,0,1,(char*)"");
	fPkt->send(sendFd,target_ai);
	seq_set.insert(fPkt->SEQ);
	thread* ft=new thread(checkTimeOut,fPkt,sendFd,target_ai,&seq_set);

	Packet rPkt=Packet();
	while(1){
		rPkt.receive(listenFd,&their_addr);
		if(rPkt.ACK==nextSeq(seqNum,1)){
			seq_set.erase(fPkt->SEQ);
			ft->join();
			break;
		}
	}
	
	// printf("HIHIHI\n");
}

Server::~Server(){
	if(hsThread.joinable()){
		hsThread.join();
	}
	freeaddrinfo(target_ai);
    close(listenFd);
    close(sendFd);
}



// srand(time(NULL))