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
            auto metadata = std::make_shared<torrent>(data);
            auto session=std::make_shared<torrent_session>(metadata);
            create_placeholder_file(session.get());  //it does return an int but lets ingonre for now
            sessions.push_back(session);
            auto tracker=std::make_shared<trackerclient>(metadata,id);
            session->get_clients();
            session->get_connections();
        }


    private:
        std::vector<std::shared_ptr<torrent_session>> sessions;
        std::string id=generate_binary_peer_id();
        
        




};









#endif