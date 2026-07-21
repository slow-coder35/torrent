#ifndef PEERCONNECTION_H
#define PEERCONNECTION_H



#include"peer_info.h"
#include "torrent.h"
#include"networking.h"
// #include "torrent_session.h"
#include"misc.h"

                 //have to figure oiut why is it needed even after include torrent_session.h
class torrent_session;

class peerconnection{
    public:
        peerconnection(const peerinfo& p,std::shared_ptr<torrent> torr,torrent_session *t): p(p), torr(torr),t(t){
           
        }
        peerconnection(std::shared_ptr<torrent> torr):torr(torr){}




        void communication();



        bool connect();

        bool is_alive();

        peerinfo pinfo(){
            return p;
        }

    

    private:
        peerinfo p;
        std::shared_ptr<torrent> torr;

        bit_f pbitfield;
        std::vector<uint32_t> required_pieces;
        uint32_t curr_idx;//idx in required_pieces not piece no
        torrent_session* t;


        bool mintrested{false};
        bool pintrested{false};
        bool mchoking{true};
        bool pchoking{true};

        int sock_fd;
        bool alive_{false};
    

    int send_handshake(const std::string& self_peer_id);

    int recieve_handshake();
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
    
    void request_piece();
    std::string req_msg();

};




#endif