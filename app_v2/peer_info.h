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

        std::string id;
        std::string ip;
        uint16_t    port;


    private:

};




#endif