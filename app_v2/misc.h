#ifndef MISC_H
#define MISC_H
#include <openssl/evp.h>
#include <openssl/err.h>
#include<string>
#include<vector>
#include<filesystem>
#include<fcntl.h>
#include<unistd.h>


class  piece{

    
    public:

    uint32_t id;


    
    bool to_download{true};
    bool downloaded;
    bool downloading;
    bool verifying;

};

class file{
    public:
        ~file(){
            if(filefd>=0)
            close(filefd);
        }

    std::filesystem::path path;
    // std::string name;
    uint64_t size;

    uint64_t begin;   //where the file start in all of the torrent 
    uint64_t end;  
    int filefd{-1};

    int create_file(){
        std::filesystem::create_directories(path.parent_path());

        int fd=open(path.c_str(),O_CREAT | O_RDWR,0644);
            if(fd<0){
        throw std::runtime_error ("could not create file  " + path.string() +'\n');
    }
    ftruncate64(fd,size);
    filefd=fd;
    return fd;
    }
    //we get a piece then i trasfer the write responsibility this itself it should give if a piece is contained in it and 
    //how much of that is in this 

    ssize_t write(const char* data,uint64_t length,uint64_t file_offset){                //starting index and ending index 
        ssize_t written=pwrite(filefd,data,length,file_offset);
        return written;
    }

};


inline std::string sha1_hash(const std::string& data){
    EVP_MD_CTX* ctx=EVP_MD_CTX_new();
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    EVP_DigestInit_ex(ctx, EVP_sha1(), nullptr);
    EVP_DigestUpdate(ctx, data.data(), data.size());
    EVP_DigestFinal_ex(ctx, hash, &hash_len);
    EVP_MD_CTX_free(ctx);

    return std::string(reinterpret_cast<char*>(hash), hash_len);

}




class bit_f{

    public:
        bit_f(){}

        std::vector<piece> bitfield;

        bool has(uint32_t piece_id){
            // int byte=piece_id/8;
            // int bit=piece_id%8;
            return bitfield[piece_id].downloaded;
        }

        void set(uint32_t piece_id){
            // int byte=piece_id/8;
            // int bit=piece_id%8;
            bitfield[piece_id].downloaded=true;
        }

        void unset(uint32_t piece_id){
            // int byte=piece_id/8;
            // int bit=piece_id%8;
            bitfield[piece_id].downloaded=true;
           
        }


};


#endif