

#include <string>
#include <vector>

#include "torrent.h"
#include "peerconnection.h"
#include "torrent_session.h"


// get a piece in our bitfeild

bool peerconnection::connect()
{   

    sock_fd=connect_to_host(p);

    int send_stat = send_handshake(t->peer_id);//have to put client sself peer if in the arguments
    if (send_stat == 0 || send_stat == -1)
        return false; // failed to create socket  or failed to send the handshake

    int recv_stat = recieve_handshake();
    if (recv_stat == -1)
    {
        close(sock_fd);
        return false;
    }
    alive_ = true;
    return true;
}

bool peerconnection::is_alive()
{
    return alive_;
}

int peerconnection::recieve_handshake()
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
    return 1;
}

int peerconnection::send_handshake(const std::string &self_peer_id)
{
    // send the handshake message recieve the message if successful connect it and return the peerconnection
    sock_fd = connect_to_host(p);

    if (sock_fd == -1)
    {
        return 0;
    }

    std::string handshake;

    handshake.push_back(19);            // 0x13
    handshake += "BitTorrent protocol"; // 19 bytes
    handshake.append(8, '\0');          // 8 zero bytes
    handshake += torr->info_hash();     // 20 bytes
    handshake += self_peer_id;          // 20 bytes

    int send_status = send_all(sock_fd, handshake);
    if (send_status <= 0)
    {
        // send failed
        close(sock_fd);
    }

    return send_status <= 0 ? -1 : 1;
}

void peerconnection::communication()
{
    std::string buf;

    
    int downloaded_num{0};

    while (downloaded_num<t->metadata->total_pieces())
    {   
        {
        std::lock_guard<std::mutex> guard(t->bitfield_lock);
        downloaded_num=t->downloaded_piece_count;
    }

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
            alive_ = false;
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
    std::cout<<"-------------download finished---------------\n";

}

void peerconnection::process_message(const std::string &msg)
{
    if(msg.empty()) return ; //keep alive
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
    std::cout <<"recieved unchoke\n";
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
    std::cout <<"bitfield is recieved\n";
    for (uint32_t i = 0; i < torr->total_pieces(); i++)
    {
        int byte = i / 8;
        int bit = 7 - (i % 8);

        bool has_piece = msg[1 + byte] & (1 << bit);
        pbitfield.bitfield[i].downloaded = has_piece;
        pbitfield.bitfield[i].id = i;
    }
    // compare our bitfeilds and set mintrested if he has peices we dont have

    for (int i = 0; i < torr->total_pieces(); i++)
    {
        if (!t->mbitfield.has(i) && pbitfield.has(i))
        {
            required_pieces.push_back(i);
        }
    }
    std::cout<<msg.size()<<'\n';
    mintrested = (!required_pieces.empty());
    curr_idx = 0;
    
    uint32_t len = htonl(1);
    std::string interested_msg(reinterpret_cast<char *>(&len),4);
    interested_msg.push_back(2);
    send_all(sock_fd, interested_msg);
}

void peerconnection::recieve_have(const std::string &msg)
{   

    
    uint32_t piece_index;
    std::memcpy(&piece_index, msg.data() + 1, sizeof(piece_index));
    piece_index = ntohl(piece_index);
    std::cout<<"recieved a have msg"<<" piece_id"<<piece_index<<'\n';
    pbitfield.set(piece_index);
    if (!t->mbitfield.has(piece_index) && pbitfield.has(piece_index))
    {
        required_pieces.push_back(piece_index);
    }
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

void peerconnection::request_piece()
{   



    if(t->downloaded_piece_count==t->metadata->total_pieces()) return;

    std::cout <<"requesting now\n";
    if (curr_idx < required_pieces.size() && mintrested && !pchoking)
    {

        int pid{-1};
        {
            std::lock_guard<std::mutex> guard(t->bitfield_lock);

            if (t->mbitfield.bitfield[required_pieces[curr_idx]].to_download)
            {
                t->mbitfield.bitfield[required_pieces[curr_idx]].to_download = false;
                t->mbitfield.bitfield[required_pieces[curr_idx]].downloading = true;
                pid = t->mbitfield.bitfield[required_pieces[curr_idx]].id;
                t->active_pieces.emplace(required_pieces[curr_idx], activepiece(required_pieces[curr_idx], torr->piece_length()));
                t->active_pieces.at(required_pieces[curr_idx]).buffer.resize(required_pieces[curr_idx]==t->metadata->total_pieces()-1
                            ?t->metadata->total_size()-required_pieces[curr_idx]*t->metadata->piece_length()
                            : t->metadata->piece_length()
            
                            );
            }
            else if(t->mbitfield.bitfield[required_pieces[curr_idx]].downloading){
                    pid=t->mbitfield.bitfield[required_pieces[curr_idx]].id;
            }
            else
            {
                curr_idx++;
            }
        }
        // start download of the pid in the buffer i,e send the request message to the peer  then ull get data in return by message recieve_piece;
        if (pid != -1)
            send_all(sock_fd, req_msg());
            // std::cout <<req_msg()<<'\n';
    }

    else if (curr_idx >= required_pieces.size())
    {
        // this peer connection is done for reciving
        // either move it back to peer or make it dormant
    }
}

std::string peerconnection::req_msg()
{
    uint32_t len = htonl(13);
    uint32_t piece = htonl(required_pieces[curr_idx]);
    uint32_t begin,blen;
{
    std::lock_guard<std::mutex> guard(t->bitfield_lock);
    uint32_t offset = t->active_pieces.at(required_pieces[curr_idx]).block_idx * BLOCK_LENGTH; // block_idx*block_length
     begin = htonl(offset);

    blen = htonl(std::min(BLOCK_LENGTH,static_cast<int>(t->active_pieces.at(required_pieces[curr_idx]).piece_length - offset)));
}
    std::vector<char> msg;
    msg.reserve(17);

    msg.insert(msg.end(), reinterpret_cast<char *>(&len), reinterpret_cast<char *>(&len) + 4);

    msg.push_back(6); // request ID

    msg.insert(msg.end(), reinterpret_cast<char *>(&piece), reinterpret_cast<char *>(&piece) + 4);

    msg.insert(msg.end(), reinterpret_cast<char *>(&begin), reinterpret_cast<char *>(&begin) + 4);

    msg.insert(msg.end(), reinterpret_cast<char *>(&blen), reinterpret_cast<char *>(&blen) + 4);
    return std::string(msg.begin(), msg.end());
}

bool verify_piece(uint32_t piece, const torrent_session *t)
{
    std::string expected_hash = t->metadata->sha1_piece(piece);

    std::string obtained_hash = t->active_pieces.at(piece).hash();
    
    if (expected_hash == obtained_hash)
        return true;
    return false;
}

void peerconnection::recieve_peice(const std::string &msg)
{

    uint32_t piece, begin;
    std::cout<< "reciving_piece ";
    std::memcpy(&piece, msg.data() + 1, 4);
    std::memcpy(&begin, msg.data() + 5, 4);

    piece = ntohl(piece);
    begin = ntohl(begin);
    auto &ap=t->active_pieces.at(piece);
    std::cout <<piece;

    memcpy(ap.buffer.data() + begin, msg.data() + 9, msg.length() - 9);

    ap.blocks_recieved[ap.block_idx++] = true;
    if (ap.block_idx == ap.blocks_recieved.size())
    {
        if (verify_piece(piece, t))
        {
            // move the index
            curr_idx++;
            // write it to its file
            write_to_file(piece,t,ap.buffer);//to be implemented

            {
                std::lock_guard<std::mutex> guard(t->bitfield_lock);
                t->mbitfield.bitfield[piece].downloaded = true;
                t->mbitfield.bitfield[piece].downloading = false;
                t->mbitfield.bitfield[piece].verifying = false;
                t->mbitfield.bitfield[piece].to_download = false;
                // flush the piece related variables back to default or just destroy the active piece
                t->active_pieces.erase(piece);
                std::cout <<"recieved_piece:"<<piece<<'\n';
            }
            request_piece();
            // begin to download new one
        }
        else
        {
            t->active_pieces.erase(piece);

            t->active_pieces.emplace(piece, activepiece(piece, torr->piece_length()));
        }
    }
    else
    {
        request_piece();
    }
}

void peerconnection::recieve_cancel(const std::string &msg)
{
    return;
}