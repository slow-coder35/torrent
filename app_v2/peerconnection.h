#ifndef PEERCONNECTION_H
#define PEERCONNECTION_H



#include"peer_info.h"
#include "torrent.h"
#include"networking.h"
// #include "torrent_session.h"
#include"misc.h"
#include <atomic>
#include<mutex>
#include<chrono>
#include<deque>

                 //have to figure oiut why is it needed even after include torrent_session.h
class torrent_session;

class peerconnection{
    public:
        peerconnection( peerinfo& p,std::shared_ptr<torrent> torr,torrent_session *t): p(p), torr(torr),t(t){
        //send handshake
        }
        peerconnection(std::shared_ptr<torrent> torr):torr(torr){}

        ~peerconnection();

        ConnectStatus status=ConnectStatus::IN_PROGRESS;  //default

        //void communication();

        void on_recv();
        long long int current_piece{-1};    //idk about the edge c
        bool recieve_handshake();          //going to be used in torrent session

        void connect();

        bool is_alive();

        peerinfo pinfo(){
            return p;
        }

        int sockfd(){
            return sock_fd;
        }

        void send_handshake(const std::string& self_peer_id);

    private:
        peerinfo p;
        std::shared_ptr<torrent> torr;


        std::deque<pending_messages> send_queue;
        std::mutex send_que_mtx;

        bit_f pbitfield;
       
        long long int current_piece{-1};    //idk about the edge condidtions from implicit conversion of uint32_t to long long int 
        //maybe get a flag to piece or not if i encounter that
        torrent_session* t;
        int outstanding_requests{0};
        std::string buf;
        
        std::chrono::milliseconds last_recv{0};

        bool mintrested{false};
        bool pintrested{false};
        bool mchoking{true};
        bool pchoking{true};

        int sock_fd;
        bool alive_{false};
    
        
   

   
    void recieve_choke();
    void recieve_unchoke();
    void recieve_intrested();
    void recieve_not_intrested();
    void recieve_have(const std::string& msg);
    void recieve_bitfeild(const std::string& msg);
    void recieve_request(const std::string& msg);
    void recieve_peice(const std::string& msg);
    void recieve_cancel(const std::string& msg);
    void process_message(const std::string& msg);
    void flush_send_buffer();
    
    void request_piece();
    std::string req_msg();

};




#endif