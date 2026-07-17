#ifndef PIECEMANAGER_H
#define PIECEMANAGER_H
#define BLOCK_LENGTH 4096

#include<cstdint>
#include <vector>
#include <mutex>
#include "networking.h"
#include "misc.h"


// #include "peerconnection.h"






class activepiece{
    public:
        activepiece(uint32_t id,uint32_t piece_length):id(id),piece_length(piece_length){
            block_count = (piece_length + BLOCK_LENGTH - 1) / BLOCK_LENGTH;
            blocks_recieved.resize(block_count);
            buffer.resize(piece_length);
        }
        // std::mutex piece_mutex;  not required rn as one piece handeld be one thread but in fufture if blocks can be requested to differnt peers it will be necessary
        uint32_t id;
        uint32_t block_count;
        uint32_t piece_length;//lonly changes if its the last block of the piece  request 
        std::vector<char> buffer;
        std::vector<bool> blocks_recieved;
        int block_idx{0};

        std::string hash() const {
                return sha1_hash(std::string (buffer.begin(),buffer.end()));
            }

};



// class piecemanager{
//     public:
//         piecemanager(){}
    
//         std::map<int ,activepiece> active_pieces;
    
//     private:
//         std::shared_ptr<torrent_session> t;




// };






#endif