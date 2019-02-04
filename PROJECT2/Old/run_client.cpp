#include <iostream>
#include "Client.h"
using namespace std;

#define CLIENT_PORT "10089"

int main(int argc, char const *argv[])
{
	char buffer[256];
    if (argc < 4) {
       fprintf(stderr, "Usage: %s hostname port filename\n", argv[0]);
       exit(0);
    }
    string file_name(argv[3]);
    string server_port(argv[2]);
    string host_name(argv[1]);
    string client_port(CLIENT_PORT);

    Client client(client_port,host_name,server_port,file_name);
    client.executeHandShake();
    client.getFile();
    client.closeConnection();

	return 0;
}