#include <iostream>
#include "Server.h"
using namespace std;
#define CLIENT_PORT "10089"

int main(int argc, char const *argv[])
{
	if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }
	string server_port(argv[1]);
	Server server(server_port,string(CLIENT_PORT));
	server.executeHandShake();
	server.sendFile();
	server.closeConnection();
	return 0;
}