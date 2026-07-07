#ifndef TORRENTSESSION_H
#define TORRENTSESSION_H

#include "torrent.h"
#include "peerconnection.h"
#include "tracker_client.h"
#include"peer_info.h"



class torrent_session{
    public:

    //open torret file

    //pass it to torrent header for processing and giving out metadata 

    //pass i tti tracker_client where the get req is sent 
    ////should be handeld by downloadeder or one more level up not sure 

    //write a function to obtain peer_connection and handshake in this file itself 







    //iterate the vector add features so that it can download and upload stuff 
    //use network.h and downloadmanager.h to comlete the task 
    //now pass the buffer once over to write file from peerconnection for every request 

    
    
    private:
    std::shared_ptr<torrent> metadata;
    std::vector<peerconnection> peer_connections;
    trackerclient client;
    int connections;

    
    
    int get_connections(){

        for (auto p : client.peer_list){
            peerconnection temp(p,metadata);
            if(temp.connect()) peer_connections.push_back(temp);


        }
    }



};



#endif