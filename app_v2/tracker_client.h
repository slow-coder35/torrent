#ifndef TRACKER_CLIENT_H
#define TRACKER_CLIENT_H

#include"torrent.h"
#include"networking.h"

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
    


    private:
    std::shared_ptr<torrent> torr;
    std::string self_peer_id;//placeholder for now comes in downloadmanager or master afaik one for all the torrents wherever it is
    



                                     
    int send_get_request_get_peers(){                    //maybe needs a refactor or splitting will see when implenting udp(maybe??)

    auto host=parse_url(torr->announce());           //get the url from torrent announce feild
    auto sh=sha1_hash(torr->info_value());           //hash all the sha1 hashes in pieces:
                                 //total length of the torrent
  

    std::string GET_REQ = 
    "GET " + host.path + "?info_hash=" + encode_url(sh) +
    "&peer_id=" + encode_url(self_peer_id) +
    "&port=6881" +
    "&uploaded=0" +                                                             /////builds the get requests 
    "&downloaded=0" +
    "&left=" + std::to_string(torr->total_size()) +
    "&compact=1" +
    " HTTP/1.1\r\n" +
    "Host: " + std::string(host.host) + "\r\n" +
    "Connection: close\r\n" +
    "\r\n";

    peerinfo temp;
    temp.ip=host.host;                                                            //connect to the host server which distributes peers
    temp.port=static_cast<uint16_t>(std::stoi(host.port));


    int sockfd=connect_to_host(temp);                                               //needs error handling

    std::string response;                                                            //reciving the message from the host containing responses in the form of 6 bytes per peers 
    int recv_status=recv_all(sockfd,response);                                        //4byte ip+ 2byte port
    
    auto start=response.find("\r\n\r\n");
        if(start==std::string::npos){
            throw std::runtime_error("Invalid http response\n");
        }
    response.erase(0,start+4);

    //parse the response 
    bencodevalue res=benvaluedecode(response);
    bencodedict res_dict=std::get<bencodedict>(res.value);

    auto peers_list=res_dict["peers"];
    bencodestring peer=std::get<bencodestring>(peers_list.value);                        //extracting the peers irrrespective of their coonectivity
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
    close(sockfd);
}
};


#endif