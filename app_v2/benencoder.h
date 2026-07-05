#ifndef BENENCODER_H
#define BENENCODER_H

#include "benparser.h"





std::string encode_int(const bencodevalue& obj){
    auto t=std::get<bencodeint>(obj.value);

    return std::string{"i"+std::to_string(t)+'e'};
}
std::string encode_string(const bencodevalue& obj){
    auto t=std::get<bencodestring>(obj.value);
    int size=t.size();
    return {std::to_string(size)+":"+t};
}

std::string encode_list(const bencodevalue& obj){
    auto t=std::get<bencodelist>(obj.value);
    std::string temp{"l"};
    for(const auto& item:t){
        temp+=bencode_encode(item);
    }
    return temp+"e";
}

std::string encode_dict(const bencodevalue& obj){
    auto t=std::get<bencodedict>(obj.value);
    std::string temp{"d"};
    bencodevalue x;
    for(const auto& [key,value]:t){
        x.value=key;
        temp=temp+encode_string(x);
        temp=temp+bencode_encode(value);
    }
    return temp+"e";

}


std::string bencode_encode(const bencodevalue& obj){
    //we initialize the string and call the 1st function 
    //it will only parse one object as it will b always be a dictionary
    std::string encoded{""};
    if(std::holds_alternative<bencodeint>(obj.value)){
        encoded=encoded+encode_int(obj);
    }
    if(std::holds_alternative<bencodestring>(obj.value)){
        encoded=encoded+encode_string(obj);
    }
    if(std::holds_alternative<bencodelist>(obj.value)){
        encoded=encoded+encode_list(obj);
    }
    if(std::holds_alternative<bencodedict>(obj.value)){
        encoded=encoded+encode_dict(obj);
    }
    return encoded;
}




#endif