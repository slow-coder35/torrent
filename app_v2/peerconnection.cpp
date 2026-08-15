

#include <string>
#include <vector>

#include "torrent.h"
#include "peerconnection.h"
#include "torrent_session.h"
#include<cassert>

peerconnection::~peerconnection()
{   

    if(!pbitfield.bitfield.empty()){
    for (uint32_t i = 0; i < t->metadata->total_pieces(); i++)
    {
        if (pbitfield.has(i))
        {
            t->piece_manager.decrease(i);
        }
    }
}

    close(sock_fd);

}

// get a piece in our bitfeild




void peerconnection::connect()
{ 

auto result = connect_to_host_non_blocking(p);
    status=result.status;
    sock_fd=result.sockfd;
}





bool peerconnection::is_alive()
{
    return alive_;
}




bool peerconnection::recieve_handshake()
{
    std::string reply;
    int recv_status = recv_s(sock_fd, reply, 68);
    if (recv_status != 68)
    {
        close(sock_fd);
        // reply dosent match unsuccessful connection
        return -1;
    }

    if (reply.substr(28, 20) != torr->info_hash())
    {
        close(sock_fd);
        return -1;
    }
    p.id = reply.substr(48, 20);
        alive_ = true;
    return true;
}



void peerconnection::send_handshake(const std::string &self_peer_id)
{
    // send the handshake message recieve the message if successful connect it and return the peerconnection
    if (sock_fd == -1)
    {
        return ;
    }
    // std::cout << "queing b torrent handshake\n";
    // handshake.push_back(19);            // 0x13
    // handshake += "BitTorrent protocol"; // 19 bytes
    // handshake.append(8, '\0');          // 8 zero bytes
    // handshake += torr->info_hash();     // 20 bytes
    // handshake += self_peer_id;          // 20 bytes

    writer handshake;
    handshake.write_8(19);
    handshake.write_str("BitTorrent protocol");
    handshake.write_64(0);
    handshake.write_str(torr->info_hash());
    handshake.write_str(self_peer_id);

    pending_messages msg;
    msg.data=handshake.value();
    msg.sent=0;
    msg.process="sendhandshake";
    {
    std::lock_guard<std::mutex> guard(send_que_mtx);
    send_queue.push_back(msg);
    
    }
}


void peerconnection::on_recv()
{

    while (true)
    {   

        if(!handshake){
            std::vector<char> temp(100);
            int n=recv(sock_fd,temp.data(),temp.size(),0);
            if(n>0)buf.append(temp.data(),n);
            if(buf.size()>=68){
                std::string reply{buf.begin(),buf.begin()+68};
                buf.erase(0,68);
                if (reply.substr(28, 20) != torr->info_hash())
                {
                    close(sock_fd);
                    alive_=false;
                    return ;
                }
                p.id = reply.substr(48, 20);
                alive_ = true;
                handshake=true;
            }
        }

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
        std::array<char,16384> temp;
        int n = recv(sock_fd, temp.data(),temp.size(), 0);
        

        if (n == 0)
        {
            // Peer closed the connection
            alive_ = false;
            
            // close the connection
            break;
            //destroy the threadaswell

        }
        else if (n == -1 && (errno == EAGAIN || EWOULDBLOCK))
        {
            // nothing more rn consume the stream then push the message back
            break;
        }
        else if (n == -1)
        {
            // close the peer
            perror("recv");
            alive_=false;
            break;
        }

        buf.append(temp.data(), n);
    }
}

void peerconnection::process_message(const std::string &msg)
{
    if (msg.empty())
        return; // keep alive
    unsigned char id = msg[0];

    switch (id)
    {
    case 0:
        recieve_choke();
        break; // done
    case 1:
        recieve_unchoke();
        break; // unchoke   //done
    case 2:
        recieve_intrested();
        break; // intrested  done
    case 3:
        recieve_not_intrested();
        break; // not intrested   //done
    case 4:
        recieve_have(msg);
        break; // have         //done
    case 5:
        recieve_bitfeild(msg);
        break; // bitfeild              //done
    case 6:
        recieve_request(msg);
        break; // request                //still have to implement upload logic
    case 7:
        recieve_peice(msg);
        break; // done
    case 8:
        recieve_cancel(msg);
        break; // camcel //idk what to do
    }
}

// when we recieve a messsage or send a message this thing handles the stuff related to it

void peerconnection::recieve_choke()
{
    pchoking = true;
}

void peerconnection::recieve_unchoke()
{
    pchoking = false;
    // std::cout << "recieved unchoke\n";
    request_piece();
}

void peerconnection::recieve_intrested()
{
    pintrested = true;
}

void peerconnection::recieve_not_intrested()
{
    pintrested = false;
}

void peerconnection::recieve_bitfeild(const std::string &msg)
{
    // pbitfield.bitfield.assign(msg.begin()+1,msg.end());\

    pbitfield.bitfield.resize(torr->total_pieces());

    // std::cout << "bitfield is recieved: "<<sock_fd<<std::endl;

    for (uint32_t i = 0; i < torr->total_pieces(); i++)
    {
        int byte = i / 8;
        int bit = 7 - (i % 8);

        bool has_piece = msg[1 + byte] & (1 << bit);
        pbitfield.bitfield[i].status = piece::downloaded;
        pbitfield.bitfield[i].id = i;
        t->piece_manager.add(i);
    }
    // compare our bitfeilds and set mintrested if he has peices we dont have

    // std::cout << msg.size() << '\n';
    mintrested = (!t->torrent_complete);

    writer intrested_msg;
    intrested_msg.write_32(1);
    intrested_msg.write_8(2);
    pending_messages temp;
    temp.add(intrested_msg);
    temp.process="intrested msg ";
    // send_all(sock_fd, {intrested_msg.value().begin(),intrested_msg.value().end()});

    {
        std::lock_guard<std::mutex> guard(send_que_mtx);
        send_queue.push_back(temp);
    }
}

void peerconnection::recieve_have(const std::string &msg)
{

    uint32_t piece_index;
    std::memcpy(&piece_index, msg.data() + 1, sizeof(piece_index));
    piece_index = ntohl(piece_index);
    pbitfield.set(piece_index);
    t->piece_manager.add(piece_index);
}

void peerconnection::recieve_request(const std::string &msg)
{
    // change the msg and start sending data
    uint32_t piece_no;
    memcpy(&piece_no, msg.data() + 1, 4);
    piece_no = ntohl(piece_no);
    uint32_t offset;
    memcpy(&offset, msg.data() + 5, 4);
    offset = ntohl(offset);
    uint32_t block_length;
    memcpy(&offset, msg.data() + 9, 4);
    block_length = ntohl(block_length);

    // send chunk after finding it in the file
}

void peerconnection::request_piece(){

    if(t->torrent_complete) return ;

    if(!pchoking){
        
        if(current_piece==-1){
        std::scoped_lock lock(t->active_pieces_lock,t->piece_manager.mtx);

            //acquire piecefrom piecemanager
            auto p=t->piece_manager.get_piece_unlocked(pbitfield);
            if(p.has_value()){
                current_piece=p.value();
            }
            else{
                return;
            }    
            t->piece_manager.set_downloading_unlocked(current_piece);
            t->active_pieces.emplace(current_piece,activepiece(current_piece,torr->piece_length()));
            std::cout<<"here at 332\n";
            t->active_pieces.at(current_piece).buffer.resize(current_piece == t->metadata->total_pieces() - 1 ? t->metadata->total_size() - current_piece * t->metadata->piece_length() : t->metadata->piece_length()

                );
                std::cout<<"here at 336\n";
        }
        //
        {
        std::scoped_lock lock(t->active_pieces_lock ,send_que_mtx);
        if(current_piece!=-1 && !t->active_pieces.at(current_piece).block_manager.asked_all){
            pending_messages msg;
            writer temp;

            temp.write_str(req_msg());
            msg.add(temp);
                msg.process="requesting piece\n";
                send_queue.push_back(msg);
                outstanding_requests++;
            
        }
    }
        
        if(outstanding_requests<40 && !t->active_pieces.at(current_piece).block_manager.asked_all) request_piece();
    }
    else {
        //close_conection
    }



}




std::string peerconnection::req_msg()
{
    
    uint32_t begin, blen;
    {
        // std::lock_guard<std::mutex> guard(t->bitfield_lock); redundant as i always ceertainly lock this in the caller request_piece 

        uint32_t offset = t->active_pieces.at(current_piece).block_manager.get_next_block()*BLOCK_LENGTH; // block_idx*block_length
        begin=offset;
        blen = std::min(BLOCK_LENGTH, static_cast<int>(t->active_pieces.at(current_piece).piece_length - offset));
    }

    writer message;
    message.write_32(13);
    message.write_8(6);
    message.write_32(current_piece);
    message.write_32(begin);
    message.write_32(blen);

    return std::string(message.value().begin(), message.value().end());  //redundant letsee
}

bool verify_piece(uint32_t piece,  torrent_session *t)
{

    std::lock_guard<std::mutex> guard(t->bitfield_lock);
    std::string expected_hash = t->metadata->sha1_piece(piece);

    std::string obtained_hash = t->active_pieces.at(piece).hash();

    if (expected_hash == obtained_hash){
        return true;
    }
    return false;

    //i have to announce to all my peer that i have the piece aswell

}



void peerconnection::recieve_peice(const std::string &msg)
{

    outstanding_requests--;

    uint32_t piece, begin;
    
    

    std::memcpy(&piece, msg.data() + 1, 4);
    std::memcpy(&begin, msg.data() + 5, 4);

    piece = ntohl(piece);
    
    // std::cout
    // << "[fd=" << sock_fd
    // << "] PIECE "
    // << piece
    // << " begin=" << begin
    // << '\n';
    // begin = ntohl(begin);

    int block_idx=(piece*t->metadata->piece_length()-begin)/BLOCK_LENGTH;
    std::cout <<"here"<<std::endl;
    
    std::lock_guard<std::mutex> guard(t->active_pieces_lock);
    auto &ap = t->active_pieces.at(piece);
    
    memcpy(ap.buffer.data() + begin, msg.data() + 9, msg.length() - 9);
    
    
    t->downloaded_bytes.fetch_add(
    msg.length()-9,
    std::memory_order_relaxed
);


  
    ap.block_manager.mark_block(block_idx);
    if (ap.block_manager.complete)
    {
        if (verify_piece(piece, t))
        {
            // write it to its file
            t->filemanager.write_piece(piece, ap.buffer);
            ap.verified = true;
            
            // update the original beitfield and erase the piece from
            {
                std::cout << current_piece <<": verified"<<std::endl;
                // std::lock_guard<std::mutex> guard(t->bitfield_lock);
                std::scoped_lock lock(t->bitfield_lock,t->piece_manager.mtx);
                t->piece_manager.set_mbitfield_unlocked(piece);
                t->downloaded_piece_count++;
                std::clog<<"downloaded pieces :"<<t->downloaded_piece_count<<"/"<<t->metadata->total_pieces()<<std::endl;
                if(t->downloaded_piece_count==t->metadata->total_pieces()) t->torrent_complete=true;
                // flush the piece related variables back to default or just destroy the active piece
                t->active_pieces.erase(piece);
                std::cout <<"erased_piece:"<< piece<<std::endl;

                
                // std::cout << "recieved_piece:" << piece << '\n';
            }
            current_piece=-1;
        }
        else
        {
            // remove it from active piece free the memory from downloading to to_download
            std::scoped_lock lock(t->bitfield_lock,t->piece_manager.mtx);
            std::cout <<"i was here\n"<<std::endl;
            t->active_pieces.erase(piece);
            t->piece_manager.unset_bitfield_unlocked(piece);
            current_piece=-1;
        }

    }

    request_piece();
}

void peerconnection::recieve_cancel(const std::string &msg)
{
    return;
}



void peerconnection::flush_send_buffer(){

    std::lock_guard<std::mutex> guard(send_que_mtx);

    while(true){
    if(send_queue.empty()) return;
    auto& curr=send_queue.front();
    
    int s=send(sock_fd, curr.data.data()+curr.sent, curr.data.size()-curr.sent, 0);
    if(s==-1){
        if(errno==EAGAIN || errno==EWOULDBLOCK){
        return ;
        }
        else {
            //close the peer connection
        }

    }
    curr.sent+=s;
    if(curr.sent==curr.data.size()){
        std::cout << "FD["<<sock_fd<<"] " <<curr.process<< std::endl;
        send_queue.pop_front();
    }

    }

}