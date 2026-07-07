#ifndef PEERCONNECTION_H
#define PEERCONNECTION_H

#include"peer_info.h"
#include "torrent.h"
#include"networking.h"


class peerconnection{
    public:
        peerconnection(const peerinfo& p,std::shared_ptr<torrent> torr): p(p), torr(torr){
           
        }
        peerconnection(std::shared_ptr<torrent> torr):torr(torr){}

        


        bool connect(){
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

        bool is_alive(){
            return alive_;
        }

    

    private:
        peerinfo p;
        std::shared_ptr<torrent> torr;

        std::vector<bool> bitfield; 
        

        bool mintrested{false};
        bool pintrested{false};
        bool mchoked{true};
        bool pchoked{true};

        int sock_fd;
        bool alive_{false};
    

    int send_handshake(const std::string& self_peer_id){
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

    int recieve_handshake(){
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


};




#endif