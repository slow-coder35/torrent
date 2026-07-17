#ifndef TORRENTSESSION_H
#define TORRENTSESSION_H
#define BLOCK_LENGTH 4096


#include "torrent.h"
#include "peerconnection.h"
#include "tracker_client.h"
#include"peer_info.h"
#include <mutex>
#include"piecemanager.h"


// struct  piece{

//     uint32_t id;
    
//     bool to_download{true};
//     bool downloaded;
//     bool downloading;
//     bool verifying;

// };

#include "file_io.h"
class bit_f;
class torrent_session;




class torrent_session{
    
    
    
    public:
        torrent_session(std::shared_ptr<torrent> metadata):metadata(metadata){
            t=this;
            peer_id=generate_binary_peer_id();
            client=trackerclient(metadata,peer_id);
            mbitfield.bitfield.resize(metadata->total_pieces());
        }

        ~torrent_session(){
            if(opfd>=0)
            close(opfd);
        }


        bit_f mbitfield;
        std::mutex bitfield_lock;
        std::map <int,activepiece> active_pieces;
        std::shared_ptr<torrent> metadata;
        int opfd{-1};//output file discriptor
        std::string peer_id;

    //pass it to torrent header for processing and giving out metadata  //donr
        

    //pass i tti tracker_client where the get req is sent  //done
    ////should be handeld by downloadeder or one more level up not sure  //done 

    //write a function to obtain peer_connection and handshake in this file itself
    int get_connections(){
        int i=0;
        for (auto p : client.peer_list){
            std::cout <<"ip:"<<p.ip<<'\n' <<"\n";   //log lines nothing of value
            peerconnection temp(p,metadata,t);
            if(temp.connect()) {peer_connections.push_back(temp);i++;
                std::cout<<" is connected\n";

            temp.communication();
            }

        }
        connections=i;
        return i;
    }

    void get_clients(){
        client.get_peer_list();
    }

    //iterate the vector add features so that it can download and upload stuff 
    //use network.h and downloadmanager.h to comlete the task 
    //now pass the buffer once over to write file from peerconnection for every request 

    
    
    private:
    
    std::vector<peerconnection> peer_connections;
    trackerclient client;
    torrent_session *t;
    int connections;
};



#endif