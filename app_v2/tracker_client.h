#ifndef TRACKER_CLIENT_H
#define TRACKER_CLIENT_H

#include"torrent.h"
#include"networking.h"
#include"misc.h"



class trackerclient{

    public:
    trackerclient(std::shared_ptr<torrent> torr,std::string id): torr(torr),self_peer_id(id){}

    trackerclient(){}
    //send get request 
    void get_peer_list(){
        send_get_request_get_peers();
    }
    //connect to host 
    //return vector<peerinfo>
    std::vector<peerinfo> peer_list; //so that it can be used in the torrent_session
    //add a method to get peerid
    const std::string& my_peer_id(){
        return self_peer_id;
    } 


    private:
    std::shared_ptr<torrent> torr;
    std::string self_peer_id;//placeholder for now comes in downloadmanager or master afaik one for all the torrents wherever it is
    uint32_t key=random();



                                     
    int send_get_request_get_peers(){         //this function does exclty what i want              //maybe needs a refactor or splitting will see when implenting udp(maybe??)

    auto host=parse_url(torr->announce());           //get the url from torrent announce feild
    /*
        sometimes url does not have a port for now if it dosent we go default
        https=443
        http=80
    */


    if(host.port.empty()){
        if(host.scheme=="http") host.port="80";
        if(host.scheme=="https") host.port="443";
    }

    peerinfo temp;
    temp.ip=host.host;
    temp.port = static_cast<uint16_t>(std::stoi(std::string(host.port)));

    auto sh=torr->info_value();           //hash all the sha1 hashes in pieces:
                                 //total length of the torrent
    std::string GET_REQ;
   
    if(host.scheme=="http" || host.scheme=="https"){
     GET_REQ = 
    "GET " + host.path + "?info_hash=" + encode_url(sh) +
    "&peer_id=" + encode_url(self_peer_id) +
    "&port=6881" +
    "&uploaded=0" +                                                             /////builds the get requests 
    "&downloaded=0" +
    "&left=" + std::to_string(torr->total_size()) +
    "&compact=1" +
    " HTTP/1.1\r\n" +             //ill have to change this for https and make a reciving type 
    "Host: " + std::string(host.host) + "\r\n" +
    "Connection: close\r\n" +
    "\r\n";


    
    //////



    //needs refactoring 
    //should go into http or https 

    }
    std::string response;

    
    if (host.scheme == "http") {
        response = handshake_http(temp, GET_REQ);
            auto start=response.find("\r\n\r\n");
        if(start==std::string::npos){
            throw std::runtime_error("no http response\n");
        }
    response.erase(0,start+4);
    }
    else if (host.scheme == "https") {
        response = handshake_https(temp, GET_REQ);
        auto start=response.find("\r\n\r\n");
        if(start==std::string::npos){
            throw std::runtime_error("no https response\n");
        }
    response.erase(0,start+4);
    }
    else  if(host.scheme=="udp"){

        response = udp_clients(temp);
        if(response.size()==0){
            std::runtime_error (" no seeders found\n");
        }
    }
    else {
        throw std::runtime_error ("protocol not supported\n");
    }



 
bencodestring peer;
    
    //parse the response  only for http or https only it is bencoded udp is not
if(host.scheme=="http"||  host.scheme=="https"){
    bencodevalue res=benvaluedecode(response);
    bencodedict res_dict=std::get<bencodedict>(res.value);

    auto peers_list=res_dict["peers"];
    peer=std::get<bencodestring>(peers_list.value); //extracting the peers irrrespective of their coonectivity
}
else if(host.scheme=="udp"){
    peer = response;
}
    
    
    
    int peer_list_length=peer.size();
    int peer_count=peer.size()/6;
    int offset{0};
    peer_list.resize(peer_count);                                                         //emliminating extra resizing time overhead by exactly setting the vector 

    for(int i=0;i<peer_count;i++){                                                      
        char ip[INET_ADDRSTRLEN];                                                          //for ipv6 just set it to INET6_ADDRSTRLEN  and AF_IPV6
         uint16_t port;

        inet_ntop(AF_INET,offset+peer.data(),ip,INET_ADDRSTRLEN);                    

        std::memcpy(&port, peer.data() + offset + 4, sizeof(port));
        port = ntohs(port);

        peer_list[i].ip=ip;
        peer_list[i].port=port;

        offset+=6;
    }
    
    return 1;
}

std::string udp_clients(peerinfo& peer){
    int sockfd=udp_connect(peer);
    
    
    
    uint64_t protocol_id = 0x41727101980ULL;

    uint32_t action = 0;
    uint32_t transaction_id = random();
    action=htonl(action);
    transaction_id=htonl(action);


    writer initiate;
    initiate.write_64(protocol_id);
    initiate.write_32(action);
    initiate.write_32(transaction_id);


    int send_st=send(sockfd, initiate.value().data(), initiate.size(), 0);

    // std::uint8_t buffer[16];
    // ssize_t n = recv(sockfd, buffer, sizeof(buffer), 0);

    char resp[16];
    ssize_t n=recv(sockfd, resp, 16, 0);

    uint32_t action_recieved;
    memcpy(&action_recieved, resp, 4);
    action_recieved = ntohl(action_recieved);
    uint32_t transaction_id_recieved;
    memcpy(&transaction_id_recieved, resp + 4, 4);
    transaction_id_recieved = ntohl(transaction_id_recieved);

    if (action_recieved != 0 || transaction_id_recieved != transaction_id)
    {
        close(sockfd);
        return "";
    }
uint64_t connection_id;
memcpy(&connection_id,resp+8,8);
connection_id=be64toh(connection_id);

/*
build udp  message to be sent
| Offset | Size | Field                                 |
| -----: | ---: | ------------------------------------- |
|      0 |    8 | `connection_id`                       |
|      8 |    4 | `action` (`1`)                        |
|     12 |    4 | `transaction_id`                      |
|     16 |   20 | `info_hash`                           |
|     36 |   20 | `peer_id`                             |
|     56 |    8 | `downloaded`                          |
|     64 |    8 | `left`                                |
|     72 |    8 | `uploaded`                            |
|     80 |    4 | `event`                               |
|     84 |    4 | `IP address` (usually `0`)            |
|     88 |    4 | `key`                                 |
|     92 |    4 | `num_want` (`0xFFFFFFFF` for default) |
|     96 |    2 | `port`                                |

*/



writer message;
message.write_64(connection_id);
message.write_32(1);
transaction_id=random();
message.write_32(transaction_id);
message.write_str(torr->info_hash());
message.write_str(self_peer_id);
message.write_64(0);                 //downloaded
message.write_64(torr->total_size());   //total size left for download
message.write_64(0);                        //total uploaded     
message.write_32(2);                          //2 for now means just started
message.write_32(0);                  //my ip address
message.write_32(key);
message.write_32(-1);                 //default = all the peers
message.write_16(6881);                //default as of nows


auto send_stat=send(sockfd,message.value().data(),message.size(),0);

char buf[4096];

auto recv_stat=recv(sockfd,buf,4096,0);

if(recv_stat<0){
    throw std::runtime_error ("no response\n");
}


//   action(32 bit)   trascaction_id (32)  interval32 leechers32 seeders32  peerinfo

//parse the response 

memcpy(&transaction_id_recieved,buf+4,4);
transaction_id_recieved=ntohl(transaction_id_recieved);
memcpy(&action_recieved,buf,4);
action_recieved=ntohl(action_recieved);
if(transaction_id!=transaction_id_recieved){
    std::runtime_error("invalid response\n");
}
std::string response(reinterpret_cast<char*>(buf + 20),recv_stat - 20);
return response;
}






};


#endif