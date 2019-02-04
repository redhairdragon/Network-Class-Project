#include "fileIO.h"
int main(int argc, char const *argv[])
{
	char buf[MAX_PAYLOAD_SIZE+1];
	buf[MAX_PAYLOAD_SIZE]='\0';
	fileIO fio((char*)"x");
	while(fio.iseof()==0){
		int nbyte=fio.nextBatch(buf,MAX_PAYLOAD_SIZE);
		buf[nbyte]='\0';
		for (int i=0;i<nbyte;i++)
			printf("%d\n", buf[i]);
		std::cout<<buf;
	}
	return 0;
}