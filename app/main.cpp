#include "benparser.h"
#include <iostream>
#include<sstream>
#include"benencoder.h"
#include<string_view>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <fstream>
#include<random>
#include <iomanip>

#include "client.h"
//peer structs 
struct Peer{
    std::string ip;
    uint16_t port;
};

//peer connection struct 

class PeerConnection{
    public:
        Peer info;
        std::string id{20};
        int sock_fd;

        bool am_choking;
        bool peer_choking;

        bool am_intrested;
        bool peer_intrested;

    
};

std::string generate_binary_peer_id() {
    std::vector<uint8_t> peer_id(20);
    
    std::random_device rd;
    // Using independent_bits_engine to cleanly feed a full byte of randomness at a time
    std::independent_bits_engine<std::mt19937, 8, unsigned char> bit_eng(rd());
    
    for (size_t i = 0; i < 20; ++i) {
        peer_id[i] = bit_eng();
    }
    
    return std::string(peer_id.begin(),peer_id.end());
}




std::string open_torrent_file(const std::string& path){
    std::ifstream file(path,std::ios::binary);
    if(!file){
        throw std::runtime_error("failed to open file\n");
    }
    file.seekg(0,std::ios::end);
    std::size_t size=file.tellg();
    file.seekg(0,std::ios::beg);

    std::string data(size,'\0');
    file.read(data.data(),size);
    return data;
}

//get the file into a bencoded value and then parse the announce list;
std::string encode_url(const std::string& hash){
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

//parsing url

struct url_parts{
    std::string scheme;
    std::string host;
    std::string port;
    std::string path;
};

struct url_parts parse_url(std::string_view url_str){
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
    url_str.remove_prefix(pt+1);

    auto pth=url_str.find('/');
    if(pth==std::string::npos) throw std::runtime_error("Invalid Url\n");
    ret.path=url_str;

    return ret;
}   

std::string sha1_hash(const std::string& data){
    EVP_MD_CTX* ctx=EVP_MD_CTX_new();
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    EVP_DigestInit_ex(ctx, EVP_sha1(), nullptr);
    EVP_DigestUpdate(ctx, data.data(), data.size());
    EVP_DigestFinal_ex(ctx, hash, &hash_len);
    EVP_MD_CTX_free(ctx);

    return std::string(reinterpret_cast<char*>(hash), hash_len);

}

std::vector<PeerConnection> get_peer_connections(bencodedict res_dict,const std::string& info_hash,const std::string& peer_id){
     auto peers_list=res_dict["peers"];
    bencodestring peer=std::get<bencodestring>(peers_list.value);
    int peer_list_length=peer.size();
    int peer_count=peer.size()/6;
    int offset{0};
    std::vector<struct Peer> peers{peer_count};

    for(int i=0;i<peer_count;i++){
        char ip[INET_ADDRSTRLEN];
         uint16_t port;

        inet_ntop(AF_INET,offset+peer.data(),ip,INET_ADDRSTRLEN);

        std::memcpy(&port, peer.data() + offset + 4, sizeof(port));
        port = ntohs(port);

        peers[i].ip=ip;
        peers[i].port=port;

        offset+=6;
    }
       std::vector<PeerConnection> peer_connections;
    PeerConnection temp{};
    for(int i=0;i<peer_count ;i++){
            
            temp.sock_fd=connect_to_host(peers[i].ip,std::to_string(peers[i].port));
            if(temp.sock_fd==-1){
                continue;
            }

           //send handshake
            std::string handshake;

            handshake.push_back(19);                    // 0x13
            handshake += "BitTorrent protocol";         // 19 bytes
            handshake.append(8, '\0');                  // 8 zero bytes
            handshake += info_hash;                     // 20 bytes
            handshake += self_peer_id;                       // 20 bytes

            int send_status=send_all(temp.sock_fd,handshake);
            if(send_status<=0){
                //send failed 
                    close(temp.sock_fd);
                    continue;
            }
            std::string reply;
            int recv_status=recv_all(temp.sock_fd,reply);
            if(recv_status!=68){
                close (temp.sock_fd);
                continue;
            }


            if(reply.substr(28,20)!=info_hash) {
                close(temp.sock_fd);
                continue;
            }
            temp.id=reply.substr(48-20,20);
            peer_connections.push_back(temp);

    }
    return peer_connections;
    
}


auto self_peer_id=generate_binary_peer_id();

int main(){

    

    struct info_start_end i;
    auto path="/home/bae/Desktop/torrent/app/Forza Horizon 6 [FitGirl Repack].torrent";
    const std::string data=open_torrent_file(path);
    bencodevalue x=benvaluedecode(data,&i);
    struct url_parts host{};
    bencodedict dict;
    //extracting url form .torrent 
    if(std::holds_alternative<bencodedict>(x.value)){
        dict=std::get<bencodedict>(x.value);
        bencodevalue url=dict["announce"];
        std::string url_str=std::get<bencodestring>(url.value);
        host=parse_url(url_str);
    }
    else return 0;    //maybe go for iteration of announce-list  //to be implemented or thought of
    



    //we got the host aswell now the sha-1 hash of info
    std::string info_bytes=data.substr(i.start,i.end-i.start);
    std::string sh=sha1_hash(info_bytes);
    

    //setting up for connection
    std::string peer_id{20};


    
    bencodevalue i_dict=dict["info"];
    bencodedict info_dict=std::get<bencodedict>(i_dict.value);


long long total_size = 0;
if (info_dict.count("length")) {
    // single-file case
    total_size = std::get<bencodeint>(info_dict["length"].value);
} else if (info_dict.count("files")) {
    // multi-file case
    bencodelist files = std::get<bencodelist>(info_dict["files"].value);
    for (auto& f : files) {
        bencodedict file_dict = std::get<bencodedict>(f.value);
        total_size += std::get<bencodeint>(file_dict["length"].value);
    }
}

    std::string GET_REQ = 
    "GET " + host.path + "?info_hash=" + encode_url(sh) +
    "&peer_id=" + encode_url(peer_id) +
    "&port=6881" +
    "&uploaded=0" +
    "&downloaded=0" +
    "&left=" + std::to_string(total_size) +
    "&compact=1" +
    " HTTP/1.1\r\n" +
    "Host: " + std::string(host.host) + "\r\n" +
    "Connection: close\r\n" +
    "\r\n";
    
    int sockfd=connect_to_host(host.host,host.port);    
    int status=send_all(sockfd,GET_REQ);
    
    std::string response;
    int recv_status=recv_all(sockfd,response);
    
    int start=response.find("\r\n\r\n");
    response.erase(0,start+4);

    //parse the response 
    bencodevalue res=benvaluedecode(response);
    bencodedict res_dict=std::get<bencodedict>(res.value);
    
    //auto get list of working peers 
    auto peer_connections=get_peer_connections(res_dict,sh,peer_id);


    //we connect the peers with us(application or user using std peer wire protocol)
    






}