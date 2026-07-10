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



class bit_f{

    public:
        bit_f(){}

        std::vector<piece> bitfield;

        bool has(uint32_t piece_id){
            int byte=piece_id/8;
            int bit=piece_id%8;
            return bitfield[byte].id & (1<<(7-bit));
        }

        void set(uint32_t piece_id){
            int byte=piece_id/8;
            int bit=piece_id%8;
            bitfield[byte].id=bitfield[byte].id | (1<<(7-bit));
        }

        void unset(uint32_t piece_id){
            int byte=piece_id/8;
            int bit=piece_id%8;
            bitfield[byte].id&=~(1<<(7-bit));
        }


};




class torrent_session{
    
    
    
    public:
        torrent_session(std::shared_ptr<torrent> metadata):metadata(metadata){
            t=this;
        }
        bit_f mbitfield;
        std::mutex bitfield_lock;
        std::map <int,activepiece> active_pieces;
        std::shared_ptr<torrent> metadata;

    //open torret file

    //pass it to torrent header for processing and giving out metadata 

    //pass i tti tracker_client where the get req is sent 
    ////should be handeld by downloadeder or one more level up not sure 

    //write a function to obtain peer_connection and handshake in this file itself


    //iterate the vector add features so that it can download and upload stuff 
    //use network.h and downloadmanager.h to comlete the task 
    //now pass the buffer once over to write file from peerconnection for every request 

    
    
    private:
    
    std::vector<peerconnection> peer_connections;

    trackerclient client;
    torrent_session *t;
    
    int connections;

    

    int get_connections(){
        int i=0;
        for (auto p : client.peer_list){
            peerconnection temp(p,metadata,t);
            if(temp.connect()) {peer_connections.push_back(temp);i++;}
            temp.communication();


        }
        return i;
    }



};



#endif