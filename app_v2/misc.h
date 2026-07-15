#ifndef MISC_H
#define MISC_H
#include <openssl/evp.h>
#include <openssl/err.h>
#include<string>
#include<vector>


class  piece{

    
    public:

    uint32_t id;


    
    bool to_download{true};
    bool downloaded;
    bool downloading;
    bool verifying;

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