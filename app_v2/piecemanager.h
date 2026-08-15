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
#include<deque>


// #include "peerconnection.h"



    enum status{
        to_request,
        requested,
        recieved
    };

    struct blockinfo{
        status stat{to_request};
    };

class blockmanager{
    public:
    blockmanager(uint32_t piece_size,uint32_t piece_id):piece_id(piece_id){
        bitfield.resize((piece_size-1+BLOCK_LENGTH)/BLOCK_LENGTH);
    }
    // std::atomic<int> block_count{0};
    bool complete{false};
    bool asked_all{false};

   int get_next_block(){
        //just a queue for now which is next 
        for(int i=0;i<bitfield.size();i++){
            if(bitfield[i].stat==status::to_request){
                bitfield[i].stat=status::requested;
            return i;
            }
        }
        asked_all=true;
        return -1;
    }


    // int get_next_block(){

    // }

    void mark_block(int i){
        bitfield[i].stat=status::recieved;
        // if(block_count==bitfield.size()) complete=true;
    }


    
    private:

        uint32_t piece_id;
        std::vector<blockinfo> bitfield{status::to_request};
    
};



class activepiece{
    public:
        activepiece(uint32_t id,uint32_t piece_length):id(id),block_manager(piece_length,id),piece_length(piece_length){
            buffer.resize(piece_length);
        }
        // std::mutex piece_mutex;  not required rn as one piece handeld be one thread but in fufture if blocks can be requested to differnt peers it will be necessary
        uint32_t id;
        blockmanager block_manager;

        uint32_t piece_length;//lonly changes if its the last block of the piece  request 
        std::vector<char> buffer;
        // std::vector<bool> blocks_recieved;
        // int block_idx{0};
        bool verified{false};

        std::string hash() const {     
                return sha1_hash(std::string (buffer.begin(),buffer.end()));
            }

};








class piecemanager{
    public:
        piecemanager(uint32_t piece_count,uint32_t piece_length){
            mbitfield.bitfield.resize(piece_count);
        }
        // piecemanager(){}

        std::mutex mtx;
        std::mutex active_piece_mtx;
        bit_f mbitfield;
        uint32_t piece_lengt`
        
        std::unordered_map<int , activepiece> active_pieces;

        //double vector get a piece and ask the next block maintain max of 20 pieces active 

        int active_pieces_count=0;

        blockmanager block_manager;


        


        
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
                        
                        // mbitfield.bitfield[piece_idx].status=piece::downloading; // Note: this is unreachable in the original code too
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
        
    

    int decide_piece( bit_f& pbitfield){
        for(auto it=scheduler.begin();it!=scheduler.end();it++){
            for(auto it=scheduler.begin();it!=scheduler.end(); it++){
                uint32_t piece_idx=it->second;
                if(mbitfield.bitfield[piece_idx].status==piece::to_download && 
                    pbitfield.has(piece_idx)){
                        // mbitfield.bitfield[piece_idx].status=piece::downloading; // Note: this is unreachable in the original code too
                        return piece_idx;
                    }

            }
            return -1;

        }
    }

    void add_to_active_pieces(uint32_t piece){
        active_pieces.emplace({piece,activepiece(piece,)});
    }


    void add_to_active_pieces(){
        
    }




};






















#endif