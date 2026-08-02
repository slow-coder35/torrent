#ifndef PIECEMANAGER_H
#define PIECEMANAGER_H
#define BLOCK_LENGTH 4096

#include<cstdint>
#include <vector>
#include <mutex>
#include "networking.h"
#include "misc.h"
#include <queue>
#include <set>
#include <mutex>


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
        bool verified{false};

        std::string hash() const {     
                return sha1_hash(std::string (buffer.begin(),buffer.end()));
            }

};



class piecemanager{
    public:
        piecemanager(uint32_t piece_count){
            mbitfield.bitfield.resize(piece_count);
        }
        // piecemanager(){}

        std::mutex mtx;
        bit_f mbitfield;


        
        //will be used by perrconnection only 
        void add(const uint32_t piece_id){
            std::lock_guard<std::mutex> guard(mtx);

            if(scheduler.count({mbitfield.bitfield[piece_id].frequency,piece_id})){
                scheduler.erase({mbitfield.bitfield[piece_id].frequency,piece_id});
            }
             mbitfield.bitfield[piece_id].frequency++;
            scheduler.insert({{mbitfield.bitfield[piece_id].frequency,piece_id}});
        }

        


        void decrease(const uint32_t piece_id){
            std::lock_guard<std::mutex> guard(mtx);
            if(scheduler.count({mbitfield.bitfield[piece_id].frequency,piece_id})){
                scheduler.erase({mbitfield.bitfield[piece_id].frequency,piece_id});
               mbitfield.bitfield[piece_id].frequency--;
                scheduler.insert({mbitfield.bitfield[piece_id].frequency,piece_id});
            }
        }

       std::optional<uint32_t> get_piece(bit_f& pbitfield){
            std::lock_guard<std::mutex> guard(mtx);
            for(auto it=scheduler.begin();it!=scheduler.end(); it++){
                uint32_t piece_idx=it->second;
                if(mbitfield.bitfield[piece_idx].status==piece::to_download && 
                    pbitfield.has(piece_idx)){
                        
                        mbitfield.bitfield[piece_idx].status=piece::downloading;
                        return piece_idx;
                    }
            }
            return std::nullopt; 
        }


        //used at the start of the program and to change the status ofbitfield maybe on resume option
        void set_mbitfield(uint32_t piece_id){
            std::lock_guard<std::mutex> guard(mtx);
            mbitfield.set(piece_id);
        }
        void unset_bitfield(uint32_t piece_id){
            std::lock_guard<std::mutex> guard(mtx);
            mbitfield.unset(piece_id);
        }
        
        int status(uint32_t piece_idx){
            std::lock_guard<std::mutex> guard(mtx);
            return mbitfield.bitfield[piece_idx].status;
        }

        void set_downloading(uint32_t piece_idx){
            std::lock_guard<std::mutex> guard(mtx);
            mbitfield.bitfield[piece_idx].status=piece::downloading;
        }


        //will be used by perrconnection only 
        void add_unlocked(const uint32_t piece_id){
            if(scheduler.count({mbitfield.bitfield[piece_id].frequency,piece_id})){
                scheduler.erase({mbitfield.bitfield[piece_id].frequency,piece_id});
            }
             mbitfield.bitfield[piece_id].frequency++;
            scheduler.insert({{mbitfield.bitfield[piece_id].frequency,piece_id}});
        }

        void decrease_unlocked(const uint32_t piece_id){
            if(scheduler.count({mbitfield.bitfield[piece_id].frequency,piece_id})){
                scheduler.erase({mbitfield.bitfield[piece_id].frequency,piece_id});
               mbitfield.bitfield[piece_id].frequency--;
                scheduler.insert({mbitfield.bitfield[piece_id].frequency,piece_id});
            }
        }

       std::optional<uint32_t> get_piece_unlocked(bit_f& pbitfield){
            for(auto it=scheduler.begin();it!=scheduler.end(); it++){
                uint32_t piece_idx=it->second;
                if(mbitfield.bitfield[piece_idx].status==piece::to_download && 
                    pbitfield.has(piece_idx)){
                        
                        mbitfield.bitfield[piece_idx].status=piece::downloading; // Note: this is unreachable in the original code too
                        return piece_idx;
                    }
            }
            return std::nullopt; 
        }

        //used at the start of the program and to change the status ofbitfield maybe on resume option
        void set_mbitfield_unlocked(uint32_t piece_id){
            mbitfield.set(piece_id);
        }
        
        void unset_bitfield_unlocked(uint32_t piece_id){
            mbitfield.unset(piece_id);
        }
        
        int status_unlocked(uint32_t piece_idx){
            return mbitfield.bitfield[piece_idx].status;
        }

        void set_downloading_unlocked(uint32_t piece_idx){
            mbitfield.bitfield[piece_idx].status=piece::downloading;
        }



    
    private:
        std::set<std::pair<uint32_t ,uint32_t>> scheduler;    //<frequency , piece_id>
        
         


};



class piece_scheduler{

    //search the bitfield maintain an array of pieces which have corresponsding peer and the frequesncy of the required pieces
    //on calling piece_scheduler it should return me the piece that has to be downloaded by that particular peer and mark it downloading in the bit field 
    //marking can be done by peer aswell on reciving it so a peer just oasks which one next and it gets it piecemanager should handle the required io on bitfield 
    //this will be nlogn as it takes logn time to inset from queue and the only ones that are inserted are new peer_connections
    //this will have an array of pieces and thier frequencies and the priorty goes lower frequency to higher frequency 
    //all i need is to increase and decrease the piece frequency everytime i parse a have / bit field message
    //so who owns this scheduler : torrent_session (fs) 
    //i can extract the top n ppieces from scheduler and check if my peer has them and assign the first one push the peer as downloading adn this way i do not make it logn for the cost of memory 

    





};








#endif