#include "lab1.h"

using std::string;

int main(int argc, char *argv[])
{
    int newsockfd;
    socklen_t clilen;
    struct sockaddr_in cli_addr,serv_addr;
    int portno=get_portno(&argc,&argv);

    memset((char *) &serv_addr, 0, sizeof(serv_addr));  // reset memory
    // fill in address info
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    int sockfd=build_socket(portno);
    if (::bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))<0)
        error("ERROR on binding");
    listen(sockfd, 5);  // 5 simultaneous connection at most

    while(1){
        //accept connections
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");
        

        int n=1;
        string request="";
        char buffer[256];
        memset(buffer, 0, 256);
        //read client's message
        while(n>0){
            n = read(newsockfd, buffer, 256); 
            if (n < 0) error("ERROR reading from socket");
            request+=string(buffer);
            memset(buffer, 0, 256);
            if(is_request_end(request))
                break;
        }
        cout<<request;

        vector<char>* content_ptr;
        string response=generate_response(request,content_ptr);

        n = write(newsockfd,response.c_str(),response.length());
        if (n < 0) error("ERROR writing to socket");
        // cout<<"\nRESPONSE:\n"<<response<<endl;
        if(content_ptr!=NULL){
            int sent_cnt=0;
            while(sent_cnt!=content_ptr->size()){
                n = write(newsockfd,&(*content_ptr)[sent_cnt],content_ptr->size()-sent_cnt);
                if (n < 0) error("ERROR writing to socket");  
                sent_cnt+=n;
            }                     
        }
        delete(content_ptr);
        close(newsockfd);  // close connection
    }
    close(sockfd);

    return 0;
}