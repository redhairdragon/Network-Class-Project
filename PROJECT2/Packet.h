#include <string>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <set>
#include <unistd.h>
#include <thread>

#define MAX_SEQ_NUM 30720
#define MAX_PACKET_SIZE 1024
#define MAX_PAYLOAD_SIZE 1013
#define WINDOW_SIZE 5120
#define TIME_OUT 500

using std::set;
class Packet
{
public:
	Packet();
	Packet(int ack,int seq,char ack_flag,char syn_flag,char fin_flag,const char* payload);
	// ~Packet();
	void send(int sendFd,struct addrinfo * target_ai,bool show_payload,bool retrans);
	void receive(int listenFd,struct sockaddr_storage* their_addr,bool show_payload);
	int ACK;
	int SEQ;
	char ACK_flag;
	char SYN_flag;
	char FIN_flag;
    char payload[MAX_PAYLOAD_SIZE];
	
};
Packet::Packet(){
	ACK=0;SEQ=0;ACK_flag=0;SYN_flag=0;FIN_flag=0;memset(payload,0,sizeof payload);
}
Packet::Packet(int ack,int seq,char ack_flag,char syn_flag,char fin_flag,const char* pay_load){
	ACK=ack;
    SEQ=seq;
    ACK_flag=ack_flag;
    SYN_flag=syn_flag;
    FIN_flag=fin_flag;
    memcpy(payload,pay_load,MAX_PAYLOAD_SIZE);
}



void Packet::send(int sendFd,struct addrinfo * target_ai,bool show_payload=0,bool retrans=0){
	int numbytes=0;
    while(numbytes!=MAX_PACKET_SIZE){
        int received;
        if ((received = sendto(sendFd, (char*)(this+numbytes),MAX_PACKET_SIZE-numbytes,0, target_ai->ai_addr, target_ai->ai_addrlen)) 
            == -1) {
            perror("talker: sendto");
            exit(1);
        }
        numbytes+=received;
    }
    if(show_payload){
    	char content[MAX_PACKET_SIZE+1];
    	memcpy(content,payload,MAX_PAYLOAD_SIZE);
    	content[MAX_PAYLOAD_SIZE]='\0';
        printf("Send packet %d %d\nPayload: %d \n\n",SEQ,WINDOW_SIZE,*(int*)payload);
    	// printf("Send packet %d %d\nPayload: %d %s\n\n",SEQ,WINDOW_SIZE,*(int*)payload,content+4);
    }
    else{
        if(retrans)
    	   printf("Send packet %d %d Retransmission\n",SEQ,WINDOW_SIZE);
        else
            printf("Send packet %d %d\n",SEQ,WINDOW_SIZE);
    }
}

void Packet::receive(int listenFd,struct sockaddr_storage* their_addr,bool show_payload=0){
	socklen_t addr_len = sizeof (*their_addr);
	int numbytes=0;
    printf("listener: waiting to recvfrom...\n");
    while(numbytes!=MAX_PACKET_SIZE){
        int received;
        if ((received = recvfrom(listenFd, (char*)this+numbytes, MAX_PACKET_SIZE-numbytes, 0,
        (struct sockaddr *)their_addr, &addr_len)) == -1){
            perror("recvfrom");
            exit(1);
        }
        numbytes+=received;
    }
    
    printf("Receiving packet %d\n",SEQ);
    printf("ACK:%d, SEQ:%d, ACK_flag: %d, SYN_flag:%d, FIN_flag:%d\n",
        ACK, SEQ,ACK_flag,SYN_flag,FIN_flag);
    if(show_payload){
    	char content[MAX_PACKET_SIZE+1];
    	memcpy(content,payload,MAX_PAYLOAD_SIZE);
    	content[MAX_PAYLOAD_SIZE]='\0';
    	printf("Payload: %d %s\n\n",*(int*)payload,content+4);
    }
	    
}
 double diffclock(clock_t clock1,clock_t clock2)
{
    double diffticks=clock1-clock2;
    double diffms=(diffticks)/(CLOCKS_PER_SEC/1000);
    return diffms;
}


int getInitialSeq(){
	return rand()%30720;
}

int nextSeq(int SEQ,int num){
	for(int i=0;i<num;i++){
		SEQ+=1;
		if(SEQ==MAX_SEQ_NUM)
			SEQ=0;
	}
	return SEQ;
}
int nextAck(int SEQ,int num){
	for(int i=0;i<num;i++){
		SEQ+=1;
		if(SEQ==MAX_SEQ_NUM)
			SEQ=0;
	}
	return SEQ;
}

void checkTimeOut(Packet* pkt,int sendFd,struct addrinfo *target_ai,set<int>* seq_set){
    std::this_thread::sleep_for (std::chrono::milliseconds(TIME_OUT));
	while(seq_set->find(pkt->SEQ)!=seq_set->end()){
        pkt->send(sendFd,target_ai,0,1);
        std::this_thread::sleep_for (std::chrono::milliseconds(TIME_OUT));
	}
    delete pkt;
}


