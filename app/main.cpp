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

        std::vector<uint8_t> bitfeild;
        int pieces_count;

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

std::vector<PeerConnection> get_peer_connections(bencodedict res_dict,const std::string& info_hash,const std::string& peer_id,int total_pieces){
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
            (temp.bitfeild).resize(total_pieces,0);
            temp.pieces_count=total_pieces;
            temp.info=peers[i];
            peer_connections.push_back(temp);

    }
    return peer_connections;
    
}




void recieve_choke(PeerConnection& Peer){
    //should just set the PeerConnection.peer_choked to true
    Peer.peer_choking=true;
    return;
}
void recieve_unchoke(PeerConnection& Peer){
    Peer.peer_choking=false;
}
void recieve_intrested(PeerConnection& Peer){
    Peer.peer_intrested=true;
    return;
}

void recieve_not_intrested(PeerConnection& Peer){
    Peer.peer_intrested=false;
    return;
}

void set_piece(int piece_index,PeerConnection& Peer){
    auto byte=piece_index/8;
    auto bit=piece_index%8;
    Peer.bitfeild[byte]|=(1<<(7-bit));
    
}

void recieve_have(const std::string& msg,PeerConnection& Peer){
    uint32_t net_index;
    memcpy(&net_index,msg.data()+1,4);
    uint32_t piece_index=ntohl(net_index);
    set_piece(piece_index,Peer);   
}



void recieve_bitfeild(const std::string& msg,PeerConnection& Peer){
    Peer.bitfeild.assign(msg.begin()+1,msg.end());
    return;
}

inline bool has_piece(size_t piece_id,const std::vector<uint8_t>& my_bitfield){
    int byte=piece_id/8;
    int bit=piece_id%8;
    return my_bitfield[byte] & (1<<(7-bit));
}
void recieve_request(const std::string& msg,PeerConnection& Peer,const std::vector<uint8_t>& my_bitfield){
    uint32_t temp, piece_id,begin,length;
    memcpy(&temp,msg.data()+1,4);
    piece_id=ntohl(temp);
    memcpy(&temp,msg.data()+5,4);
    begin=ntohl(temp);
    memcpy(&temp,msg.data()+9,4);
    length=ntohl(temp);
    //verify if i have the piece
    if(has_piece(piece_id,my_bitfield)){
        //send the chunk routine
    }

}


void recieve_peice(const std::string& msg,PeerConnection& Peer){

}
void recieve_cancel(const std::string& msg,PeerConnection& Peer);

void process_message(std::string& msg,PeerConnection& Peer,std::vector<uint8_t>& my_bitfeild){
    unsigned char id=msg[0];

    switch(id){
        case 0: //choke
        case 1: //unchoke
        case 2: //intrested
        case 3: //not intrested
        case 4: //have
        case 5: //bitfeild
        case 6: //request
        case 7: //peice
        case 8: //cancel
    }
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
    std::string peer_id;


    
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
    
    //auto get list of working peers and connect to them  
    //peer wire protocol
     std::vector<uint8_t> my_bitfeild;
    int total_pieces{0};
    { auto piece_bencode=info_dict["piece_length"];
    auto piece_length=std::get<bencodeint>(piece_bencode.value);
    total_pieces=(total_size+piece_length-1)/piece_length;
    int bitfeild_bytes=(total_pieces+7)/8;
    my_bitfeild.resize(bitfeild_bytes,0);
    }


    auto peer_connections=get_peer_connections(res_dict,sh,peer_id,total_pieces);

   

    auto Peer=peer_connections[0];
    std::string buf,msg;
    uint32_t len{0};
    int n{1};
    std::string buf;

    while (true)
    {
    // Can we parse a complete message already?
    if (buf.size() >= 4)
    {
        uint32_t len;
        std::memcpy(&len, buf.data(), 4);
        len = ntohl(len);

        // Do we have the whole message?
        if (buf.size() >= 4 + len)
        {
            std::string msg = buf.substr(4, len);

            // Remove the processed message from the buffer
            buf.erase(0, 4 + len);

            process_message(msg,Peer,my_bitfeild);

            // another complete message
            continue;
        }
    }

    // Need more bytes
    char temp[4096];
    int n = recv(Peer.sock_fd, temp, sizeof(temp), 0);

    if (n == 0)
    {
        // Peer closed the connection
        break;
    }

    if (n < 0)
    {
        // Error
        perror("recv");
        break;
    }

    buf.append(temp, n);
}


}