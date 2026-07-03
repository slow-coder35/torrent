#ifndef CLIENT_H
#define CLIENT_H

#include<iostream>
#include<string>
#include<memory>
#include<cstring>
#include<sys/socket.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<netinet/in.h>
#include <unistd.h>
#include<optional>
#define BUFFER_LENGTH 4096


#include"benencoder.h"
#include "benparser.h"


int connect_to_host(const std::string &hostname,const std::string& port){
    struct addrinfo hint{};
    struct addrinfo *results,*p;

    hint.ai_family=AF_INET;
    hint.ai_socktype=SOCK_STREAM;

    int status=getaddrinfo(hostname.c_str(),port.c_str(),&hint,&results);
    if(status!=0){
        std::cerr<<"failed to fetch\n";
        //freeaddrinfo(results);
        return -1;
    }
    p=results;

    int c_status{-1},sockfd{-1};
    while((c_status==-1 || sockfd==-1) && p!=nullptr){
        close(sockfd);
        sockfd=socket(p->ai_family,p->ai_socktype,0);
        c_status=connect(sockfd,p->ai_addr,p->ai_addrlen);
        p=p->ai_next;    
    }

    if(c_status==-1){
        std::cerr << "failed to connect\n";
        freeaddrinfo(results);
        return -1;
    }
    freeaddrinfo(results);

    return sockfd;

}


int  recv_all(int sockfd,std::string& ret){
    
    char buffer[BUFFER_LENGTH];
    int recived_sofar=0;

    while(true){
        int ret_size=recv(sockfd,buffer,BUFFER_LENGTH,0);
        if(ret_size<=0) break;
        recived_sofar+=ret_size;
        ret.append(buffer,ret_size);
    }
    if(ret.length()<=0) return -1;
    return recived_sofar ;

}


int send_all(int sockfd, const std::string& data){
    size_t total_sent = 0;
    while(total_sent < data.length()){
        int result = send(sockfd, data.c_str() + total_sent, data.length() - total_sent, 0);
        if(result <= 0) return -1;
        total_sent += result;
    }
    return static_cast<int>(total_sent);
}

// int main(int argc,char* argv[]){
//     if(argc!=2) {
//         std::cerr<<"usage :showip hostname\n";
//         return 1;
//     }  
//     std::string hostname = argv[1];
//     std::string port="1234";

//     int sockfd=connect_to_host(hostname,port);
//     if(sockfd==-1) return 0;

//     const char* msg="hello world :p\n";
//     int ms{send_all(sockfd,msg)};
//     std::string msg1;
//     int result=recv_all(sockfd,msg1);
//     if(result==-1){
//         std::cerr<<"recieve failed\n";
//         close(sockfd);
//         return 1;
//     }
//     close(sockfd);
//     return 0;

// }

#endif
