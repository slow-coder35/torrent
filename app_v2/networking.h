#ifndef NETWORK_H
#define NETWORK_H


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
#include<random>

#include "misc.h"
#include "torrent.h"
#include "peer_info.h"                                      
//hostname=peerinfo.ip;
//port=peerinfo.port
#define BUFFER_LENGTH 4096



inline int connect_to_host(peerinfo& peer){
    struct addrinfo hint{};
    struct addrinfo *results,*p;

    hint.ai_family=AF_INET;
    hint.ai_socktype=SOCK_STREAM;

    int status=getaddrinfo(peer.ip.c_str(),std::to_string(peer.port).c_str(),&hint,&results);
    if(status!=0){
        std::cerr<<"failed to fetch\n";
        //freeaddrinfo(results);
        return -1;
    }
    p=results;

    int c_status{-1},sockfd{-1};
    while((c_status==-1 || sockfd==-1) && p!=nullptr){
        if(sockfd>0) close(sockfd);
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



inline int  recv_all(int sockfd,std::string& ret){
    
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




inline int recv_s(uint32_t sockfd,std::string& reply,int length){
    reply.clear();
    char buffer[BUFFER_LENGTH];
    int recieved_sofar=0;
    while(recieved_sofar<length){
        int ret_size=recv(sockfd,buffer,std::min(BUFFER_LENGTH,length-recieved_sofar),0);
        if(ret_size<=0) return -2;
        recieved_sofar+=ret_size;
        reply.append(buffer,ret_size);
    }
    return recieved_sofar == length ? recieved_sofar : -1;
}

inline int send_all(int sockfd, const std::string& data){
    size_t total_sent = 0;
    while(total_sent < data.length()){
        int result = send(sockfd, data.c_str() + total_sent, data.length() - total_sent, 0);
        if(result <= 0) return -1;
        total_sent += result;
    }
    return static_cast<int>(total_sent);
}





inline std::string encode_url(const std::string& hash){
    std::string ret;
    for(char c:hash){
        if (isalnum(c) || c=='-' || c=='_' || c=='.' || c=='~') {
    ret=ret+c;
} 
    else {
   char buf[4]; // "%XX\0"
    snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)c);
   ret+= buf;
}
    }
    return ret;
}



struct url_parts{
    std::string scheme;
    std::string host;
    std::string port;
    std::string path;
};




inline struct url_parts parse_url(std::string_view url_str){
    struct url_parts ret{};
    auto scheme_end=url_str.find("://");
    if(scheme_end==std::string::npos) throw std::runtime_error("Invalid URL\n");
    ret.scheme=url_str.substr(0,scheme_end);
    
    url_str.remove_prefix(scheme_end+3);

    auto slash=url_str.find(":");
    if(slash==std::string::npos) throw std::runtime_error("Invalid URL\n");
    ret.host=url_str.substr(0,slash);
    url_str.remove_prefix(slash+1);

    auto pt=url_str.find('/');
    if(pt==std::string::npos) throw std::runtime_error("Invalid URL\n");
    ret.port=url_str.substr(0,pt);
    ret.path = url_str.substr(pt);

    return ret;
}   






inline std::string generate_binary_peer_id() {
    std::vector<uint8_t> peer_id(20);
    
    std::random_device rd;
    // Using independent_bits_engine to cleanly feed a full byte of randomness at a time
    std::independent_bits_engine<std::mt19937, 8, unsigned char> bit_eng(rd());
    
    for (size_t i = 0; i < 20; ++i) {
        peer_id[i] = bit_eng();
    }
    
    return std::string(peer_id.begin(),peer_id.end());
}










// std::string build_get_request(){
    
// }

// std::vector<peerinfo> send_get_request_get_peers(const std::shared_ptr<torrent> torr,const std::string& peer_id){

//     auto host=parse_url(torr->announce());           //get the url from torrent announce feild
//     auto sh=sha1_hash(torr->info_value());           //hash all the sha1 hashes in pieces:
//                                  //total length of the torrent
  

//     std::string GET_REQ = 
//     "GET " + host.path + "?info_hash=" + encode_url(sh) +
//     "&peer_id=" + encode_url(peer_id) +
//     "&port=6881" +
//     "&uploaded=0" +
//     "&downloaded=0" +
//     "&left=" + std::to_string(torr->total_size()) +
//     "&compact=1" +
//     " HTTP/1.1\r\n" +
//     "Host: " + std::string(host.host) + "\r\n" +
//     "Connection: close\r\n" +
//     "\r\n";

//     peerinfo temp;
//     temp.ip=host.host;
//     temp.port=static_cast<uint16_t>(std::stoi(host.port));
//     int sockfd=connect_to_host(temp);

//     std::string response;
//     int recv_status=recv_all(sockfd,response);
    
//     int start=response.find("\r\n\r\n");
//     response.erase(0,start+4);

//     //parse the response 
//     bencodevalue res=benvaluedecode(response);
//     bencodedict res_dict=std::get<bencodedict>(res.value);

//     auto peers_list=res_dict["peers"];
//     bencodestring peer=std::get<bencodestring>(peers_list.value);
//     int peer_list_length=peer.size();
//     int peer_count=peer.size()/6;
//     int offset{0};
//     std::vector<peerinfo> peers{peer_count};

//     for(int i=0;i<peer_count;i++){
//         char ip[INET_ADDRSTRLEN];
//          uint16_t port;

//         inet_ntop(AF_INET,offset+peer.data(),ip,INET_ADDRSTRLEN);

//         std::memcpy(&port, peer.data() + offset + 4, sizeof(port));
//         port = ntohs(port);

//         peers[i].ip=ip;
//         peers[i].port=port;

//         offset+=6;
//     }    

//     return peers;


// }








#endif