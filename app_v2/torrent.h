#ifndef TORRENT_H
#define TORRENT_H


#include<string>
#include<vector>
#include<memory>


#include"benencoder.h"
#include"benparser.h"


struct file{
    std::string name;
    uint64_t size;

};



class torrent{
    public:


        explicit torrent(const std::string& file_input){   ///explicit so that the torrent cant be created mistakenly
            bencodevalue root_=benvaluedecode(file_input);
            auto&  root=std::get<bencodedict>(root_.value);
            //assign announce 
            assign_announce(root);
           
            //assign announce list
            assign_announce_list(root);
            //assign file_list_ 
            bencodevalue info_rt=root["info"];
            bencodedict info_root=std::get<bencodedict>(info_rt.value);
            assign_file_list(info_root);
            //assign name
            assign_name(info_root);
            //assign piecelength
            assign_piece_length(info_root);
            //assign piecehash
            assign_sha1_pieces(info_root);
        }
        

        bool multifile()const{
            return multi_file;
        }


    private:
        std::string announce_;
        std::vector<std::string> announce_list_;

        std::vector<file> file_list_; 
        file file_;

        std::string name_;

        uint32_t piece_length_;

        std::string pieces_sha1_hash_;

        bool multi_file;
        
        void assign_announce(bencodedict& root){
            bencodevalue announce=root["announce"];
            announce_=std::get<bencodestring>(announce.value);
        }

        void assign_announce_list(bencodedict& root){
            bencodevalue announce_lt=root["announce-list"];
            bencodelist announce_list=std::get<bencodelist>(announce_lt.value);
            std::string str;
            for(auto url_str : announce_list){
                std::string str=std::get<bencodestring> (url_str.value);
                announce_list_.push_back(str);
            }
        }

        void assign_file_list(bencodedict& info_root){
            if(info_root.count("files")){
                //process as a multifile torrent;
                multi_file=true;
                bencodelist file_list=std::get<bencodelist>(info_root["files"].value);
                bencodedict temp;
                uint64_t file_length;
                std::string name;
                file tp;
                for(auto url_str: file_list){
                    temp=std::get<bencodedict>(url_str.value);
                    tp.size=std::get<bencodeint>(temp["length"].value);
                    tp.name=std::get<bencodestring>(temp["path"].value);
                    file_list_.push_back(tp);
                }
            }
            else{

                multi_file=false;
                file_.size=std::get<bencodeint>(info_root["length"].value);
                file_.name=std::get<bencodestring>(info_root["name"].value);

            }
        }

        void assign_name(bencodedict& info_root){
            name_=std::get<bencodestring>(info_root["name"].value);
        }
        void assign_piece_length(bencodedict& info_root){
            piece_length_=std::get<bencodeint>(info_root["piece length"].value);
        }

        void assign_sha1_pieces(bencodedict& info_root){
            pieces_sha1_hash_=std::get<bencodestring>(info_root["pieces"].value);   
        }

};









#endif