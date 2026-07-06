#ifndef PEER_INFO_H
#define PEER_INFO_H

#include<string>
#include<vector>
#include<iostream>
#include<cstdint>

class peerinfo{
    public:
            peerinfo(const std::string& ip,const uint16_t port,const std::string id) : ip(ip),port(port),id(id){


            }
            peerinfo(const std::string& ip,const uint16_t port):ip(ip),port(port){

            }
            peerinfo(){}

            // void set_i_am_intrested(){
            //     i_am_intrested_=true;
            // }
            // void unset_i_am_intrested(){
            //     i_am_intrested_=false;
            // }

            // void set_peer_intrested(){
            //     peer_intrested_=true;
            // }
            // void unset_peer_intrested(){
            //     peer_intrested_=false;
            // }

            // void set_peer_choking(){
            //     peer_choking_=true;
            // }
            // void unset_peer_choking(){
            //     peer_choking_=false;
            // }
            // void set_i_am_choking(){
            //     i_am_choking_=true;
            // }
            // void unset_i_am_choking(){
            //     i_am_choking_=false;
            // }   

            // bool get_i_am_intrested()const{
            //     return i_am_intrested_;
            // }

            // bool get_peer_intrested()const{
            //     return peer_intrested_;
            // }
            // bool get_i_am_choking()const{
            //     return i_am_choking_;
            // }

            // bool get_peer_choking()const{
            //     return peer_choking_;
            // }
        std::string id;
        std::string ip;
        uint16_t    port;

    private:
        // bool i_am_intrested_{false};
        // bool peer_intrested_{false};

        // bool i_am_choking_{true};
        // bool peer_choking_{true};

        // std::vector<uint8_t> bitfield;
};




#endif