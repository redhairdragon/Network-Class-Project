#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>  /* signal name macros, and the kill() prototype */
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>    
#include <fcntl.h>
#include <iostream>
#include <string>
#include <string.h>
#include <fstream>
#include <sstream>
#include "UriCodec.cpp"
#include <map>
#include <vector>

using namespace std;

string lowered_string(const string& str){
    string lowered=string(str);
    for (size_t i=0; i<lowered.length(); i++)
        lowered[i]=tolower(lowered[i]);
    return lowered;
}

string get_file_extension(const string& filename){
    size_t found = filename.find_last_of(".");
    if(found == string::npos)
        return("");
    return lowered_string(filename.substr(found+1));
}


string get_file_type(const string& filename){
    string ext=get_file_extension(filename);
    if(ext=="jpg")
        return string("image/jpeg");
    if(ext=="txt")
        return string("text/plain");
    if(ext=="htm"||ext=="html")
        return string(" text/html");
    if(ext=="jpeg")
        return string("image/jpeg");
    if(ext=="gif")
        return string("image/gif");
    return string("application/octet-stream");
}

//return the filename if there is a matching file
string search_folder(const string& filename){
    string fn=lowered_string(filename);
    DIR * dir;
    struct dirent * ptr;
    int i;
    dir = opendir(".");
    while((ptr = readdir(dir)) != NULL){
        int fn_len=strlen(ptr->d_name);
        // cout<<ptr->d_name<<endl;
        if(fn_len==filename.length()){
            string lowered_curr_filename=lowered_string(string(ptr->d_name));
            if(lowered_curr_filename==fn)
                return string(ptr->d_name);
        }
    }
    closedir(dir);
    return string("");
}

long get_file_size(string filename){
    struct stat st;
    if (stat(filename.c_str(), &st) == 0)
        return st.st_size;
    return -1; 
}


vector<char>* get_file(const string& filename){
    string fn=search_folder(filename);
    if(fn.empty()==true)
        return NULL;//no file found
    size_t size=get_file_size(fn);
    ifstream fin;
    fin.open(fn);
    vector<char>* r=new vector<char>(size);
    fin.read(r->data(),size);
    return r;
}


bool is_request_end(const string& request){
    if(request.length()>4){
        if(request.substr(request.length()-4)=="\r\n\r\n")
            return true;    
    }
    return false;
}

string not_found_header(){
    return string("HTTP/1.0 404 Not Found\r\nContent-Type:text/html\r\nContent-Length: 12\r\nConnection: close\r\n\r\n<h2>404</h2>");
}


string generate_response(const string& request,vector<char>*& content){
    content=NULL;
    int start=request.find(" /");
    int end=request.find("HTTP/1.1\r\n");
    if(start==-1||end==-1)
        return not_found_header();
    string filename=UriDecode(request.substr(start+2,end-start-2-1));

    if(filename.empty())
        return not_found_header();
    string fn=search_folder(filename); 
    content=get_file(fn);
    if(content==NULL)
        return not_found_header();

    string status_line=string("HTTP/1.1 200 OK\r\n");
    string content_length=string("Content-Length:")+to_string(content->size())+string("\r\n");
    string content_type=string("Content-Type:")+get_file_type(fn)+string("\r\n");
    string connection=string("Connection: Closed\r\n\r\n");
    string header=status_line+content_length+content_type+connection;
    return header;
}

void error(string msg){
    perror(msg.c_str());
    exit(1);
}

int get_portno(int* argc, char** argv[]){
    if ((*argc) < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }
    return atoi((*argv)[1]);
}
//return socket fd
int build_socket(int portno){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);  // create socket
    if (sockfd < 0)
        error("ERROR opening socket");
    return sockfd;
}

