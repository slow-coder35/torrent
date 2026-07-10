

#include <string>
#include <vector>


#include "torrent.h"
#include "peerconnection.h"
#include "torrent_session.h"

//get a piece in our bitfeild 


bool peerconnection::connect(){
            int send_stat=send_handshake("iuguygkugu");
            if(send_stat==0 || send_stat==0) return false;  //failed to create socket  or failed to send the handshake
            
            int recv_stat=recieve_handshake();
            if(recv_stat==-1){
                close (sock_fd);
                return false;
            }
            alive_=true;
            return true;

        }

        bool peerconnection::is_alive(){
            return alive_;
        }

    
int peerconnection:: recieve_handshake(){
        std::string reply;
        int recv_status=recv_all(sock_fd,reply);
        if(recv_status!=68){
                close (sock_fd);
                //reply dosent match unsuccessful connection
                return -1;
        }


        if(reply.substr(28,20)!=torr->info_hash()) {
                close(sock_fd);
                return -1;
            }
            p.id=reply.substr(48,20);
            return 1;
    }

int peerconnection::send_handshake(const std::string& self_peer_id){
        //send the handshake message recieve the message if successful connect it and return the peerconnection 
        sock_fd=connect_to_host(p);
        
        if(sock_fd==-1){
            return 0;
        }


        std::string handshake;

            handshake.push_back(19);                    // 0x13
            handshake += "BitTorrent protocol";         // 19 bytes
            handshake.append(8, '\0');                  // 8 zero bytes
            handshake += torr->info_hash();                   // 20 bytes
            handshake += self_peer_id;                       // 20 bytes


         int send_status=send_all(sock_fd,handshake);
            if(send_status<=0){
                //send failed 
                    close(sock_fd);
            }
            return send_status<=0?-1:1;
    }


int peerconnection::communication()
        {
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

                        process_message(msg);

                        // another complete message
                        continue;
                    }
                }

                // Need more bytes
                char temp[4096];
                int n = recv(sock_fd, temp, sizeof(temp), 0);

                if (n == 0)
                {
                    // Peer closed the connection
                    alive_==false;
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





void peerconnection::process_message(const std::string& msg){
    unsigned char id=msg[0];

    switch(id){
        case 0: recieve_choke();break;   //done
        case 1: recieve_unchoke();break;//unchoke   //done
        case 2: recieve_intrested();break;//intrested  done
        case 3: recieve_not_intrested();break;//not intrested   //done
        case 4: recieve_have(msg);break;//have         //done
        case 5: recieve_bitfeild(msg);break;//bitfeild              //done
        case 6: recieve_request(msg);break;//request                //still have to implement upload logic
        case 7: recieve_peice(msg);                                 
        case 8: recieve_cancel(msg);//camcel
    }
}

//when we recieve a messsage or send a message this thing handles the stuff related to it

void peerconnection::recieve_choke(){
    pchoking=true;
}

void peerconnection::recieve_unchoke(){
    pchoking=false;
    request_piece();
}

void peerconnection::recieve_intrested(){
    pintrested=true; 
}


void peerconnection::recieve_not_intrested(){
    pintrested=false;
}



void peerconnection::recieve_bitfeild(const std::string& msg){
    pbitfield.bitfield.assign(msg.begin()+1,msg.end());
    //compare our bitfeilds and set mintrested if he has peices we dont have
    
    for(int i=0;i<torr->total_pieces();i++){
        if(!t->mbitfield.has(i) && pbitfield.has(i) ){
            required_pieces.push_back(i);
        }
    }
    mintrested=(!required_pieces.empty());
    curr_idx=0;
    std::string intrested_msg;
    intrested_msg.push_back(htonl(4));
    intrested_msg+=2;

}


void peerconnection::recieve_have(const std::string& msg){
   uint32_t piece_index;
   std::memcpy(&piece_index, msg.data() + 1, sizeof(piece_index));
   piece_index = ntohl(piece_index);
   pbitfield.set(piece_index);
}




void peerconnection::recieve_request(const std::string& msg){
    //change the msg and start sending data
    uint32_t piece_no;
    memcpy(&piece_no,msg.data()+1,4);
    piece_no=ntohl(piece_no);
    uint32_t offset;
    memcpy(&offset,msg.data()+5,4);
    offset=ntohl(offset);
    uint32_t block_length;
    memcpy(&offset,msg.data()+9,4);
    block_length=ntohl(block_length);

    //send chunk after finding it in the file

}




void peerconnection::request_piece(){
    if(curr_idx<required_pieces.size() && mintrested && !pchoking){
       
        int pid{-1};
        {
        std::lock_guard<std::mutex> guard(t->bitfield_lock);

        if(required_pieces[0] && t->mbitfield.bitfield[required_pieces[curr_idx]].to_download){
            t->mbitfield.bitfield[required_pieces[curr_idx]].to_download=false;
            t->mbitfield.bitfield[required_pieces[curr_idx]].downloading=true;
            pid=t->mbitfield.bitfield[required_pieces[curr_idx]].id;
            t->active_pieces[required_pieces[curr_idx]];
        }
        else{
            curr_idx++;
        }

    }
    //start download of the pid in the buffer i,e send the request message to the peer  then ull get data in return by message recieve_piece;
    if(pid!=-1) send_all(sock_fd,req_msg());
}



else if(curr_idx>=required_pieces.size()) {
    //this peer connection is done for reciving
    //either move it back to peer or make it dormant
}

}


std::string peerconnection::req_msg(){
uint32_t len   = htonl(13);
uint32_t piece = htonl(required_pieces[curr_idx]);


uint32_t offset=t->active_pieces[required_pieces[curr_idx]].block_idx*(BLOCK_LENGTH);   //block_idx*block_length 
uint32_t begin = htonl(offset);

uint32_t blen  = htonl(BLOCK_LENGTH);

std::vector<char> msg;
msg.reserve(17);

msg.insert(msg.end(), reinterpret_cast<char*>(&len),reinterpret_cast<char*>(&len) + 4);

msg.push_back(6);   // request ID

msg.insert(msg.end(), reinterpret_cast<char*>(&piece),reinterpret_cast<char*>(&piece) + 4);

msg.insert(msg.end(), reinterpret_cast<char*>(&begin),reinterpret_cast<char*>(&begin) + 4);

msg.insert(msg.end(), reinterpret_cast<char*>(&blen),reinterpret_cast<char*>(&blen) + 4);
return std::string(msg.begin(),msg.end());
}


bool verify_piece(uint32_t piece,const torrent_session* t){
    std::string expected_hash=t->metadata->sha1_piece(piece);
    std::string obtained_hash=t->active_pieces.at(piece).hash();
    if(expected_hash==obtained_hash) return true;
    return false;

}



void peerconnection::recieve_peice(const std::string& msg){
    
uint32_t piece, begin;

std::memcpy(&piece, msg.data() + 1, 4);
std::memcpy(&begin, msg.data() + 5, 4);

piece = ntohl(piece);
begin = ntohl(begin);

    memcpy(t->active_pieces[piece].buffer.data()+begin,msg.data()+9,msg.length()-9);

    t->active_pieces[piece].blocks_recieved[t->active_pieces[piece].block_idx++]=true;
    if(t->active_pieces[piece].block_idx==t->active_pieces[piece].blocks_recieved.size()){
        if(verify_piece(piece,t)){
            //move the index 
            curr_idx++;
            //write it to its file 
            //write_to_file;//to be implemented
            {
            std::lock_guard<std::mutex> guard(t->bitfield_lock);
            t->mbitfield.bitfield[piece].downloaded=true;
            t->mbitfield.bitfield[piece].downloading=false;
            t->mbitfield.bitfield[piece].verifying=false;
            t->mbitfield.bitfield[piece].to_download=false;
            //flush the piece related variables back to default or just destroy the active piece 
            t->active_pieces.erase(piece);
            }
            request_piece();
            //begin to download new one 


        }
        else{
            //start from first of the same piece
            //just erase the stuff and add a new activepiece initialized again
            
        }
    }
    else{
        request_piece();
    }
    

}

