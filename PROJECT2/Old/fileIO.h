#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <cstring>
#define MAX_PAYLOAD_SIZE 1013

class fileIO {
 public:
    fileIO(const char* fileName);
    int nextBatch(char* buf,size_t size);
    bool iseof(){return fin.eof();}
    ~fileIO(){fin.close();}
private:
	std::ifstream fin;
};
fileIO::fileIO(const char* fileName){
	fin.open(fileName);
	if(fin.is_open()==0)
		exit(1);
}

int fileIO::nextBatch(char* buf,size_t size){
	if(fin.eof()==0)
		fin.read(buf, size);
	else{
		memset(buf,0,size);
		return 0;
	}
	return (int)fin.gcount();
}

// #include "fileIO.h"
// int main(int argc, char const *argv[])
// {
// 	char buf[MAX_PAYLOAD_SIZE+1];
// 	buf[MAX_PAYLOAD_SIZE]='\0';
// 	fileIO fio((char*)"x");
// 	while(fio.iseof()==0){
// 		int nbyte=fio.nextBatch(buf,MAX_PAYLOAD_SIZE);
// 		buf[nbyte]='\0';
// 		std::cout<<buf;
// 	}
// 	return 0;
// }