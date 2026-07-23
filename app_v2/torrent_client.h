#ifndef TORRENT_CLIENT_H
#define TORRENT_CLIENT_H
#include"benencoder.h"
#include"benparser.h"
#include "networking.h"
#include"torrent.h"
#include"torrent_session.h"
#include "tracker_client.h"

#include <memory>
#include <vector>
#include <string>
#include <iostream>


class torrent_client{

    public:
        torrent_client (){}
        

        void download(std::string& path){
            

            std::string data=fetch_torrent_file(path);
            
            auto metadata = std::make_shared<torrent>(data);//data is passed and torrent meta data is complete
            
            auto session=std::make_shared<torrent_session>(metadata);
            //i have to get peers first 
            session->get_clients();
            // session->opfd=create_placeholder_file(session.get());   file io is now done while initializing torrent session itself
            //get peerconnection they are not created atomatically
            session->get_connections();             //it does return an int but lets ingonre for now it sets connections and starts comunicating aswell 
            //optional file verification 
            //wait for threads to end
            session->start_communication(); //startss communication
            session->wait_to_finish();
            // sessions.push_back(session);            //torrent is already done atp
        }

    private:
        std::vector<std::shared_ptr<torrent_session>> sessions;
        std::string id=generate_binary_peer_id();
        
        




};









#endif