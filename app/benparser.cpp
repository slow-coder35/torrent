#ifndef BENPARSER_H
#define BENPARSER_H


#include <string.h>
#include <vector>
#include <map> 
#include <memory>
#include <variant>
#include <cctype>
#include<stdexcept>

struct bencodevalue;
using bencodedict=std::map<std::string,bencodevalue>;
using bencodelist=std::vector<bencodevalue>;
using bencodeint=long long;
using bencodestring=std::string;




struct bencodevalue{
    std::variant<bencodeint,
        bencodestring,
        bencodelist,
        bencodedict> value;
};


//PARSE An int 
std::pair<bencodevalue,int> parse_int(std::string& data,int pos,char c)
{
    
    bencodeint number={0};
    bool isnegative=(data[pos]=='-');
    if(isnegative) pos++;
    int idx=pos;
    if(data[pos]=='0' && isnegative) throw std::runtime_error("invalid bencode");
    while(data[pos]!=c && isdigit(data[pos])){
        number=number*10+ (data[pos]-'0');
        pos++;
    }
    if((!isdigit(data[pos]) && data[pos]!=c)|| idx==pos ) throw std::runtime_error("invalid bencode");
    if(isnegative) number=-1*number;
    bencodevalue result;
    result.value=number;
    return {result,pos+1};
}


std::pair<bencodevalue,int> parse_string(std::string& data,int pos){
    bencodestring ret{""};
    
    auto [n,x]=parse_int(data,pos,':');
    pos=x;
    bencodeint m = std::get<bencodeint>(n.value);
    if(data[pos]!=':') throw std::runtime_error("invalid bencode ");
    pos++;
    for(int i=0;i<m;i++){
        ret=ret+data[pos+i];
    }
    pos=pos+m;
    bencodevalue result;
    result.value=ret;
    return {result ,pos};

}






std::pair<bencodevalue,int> parse_list(std::string& data,int pos){

}

std::pair<bencodevalue,int> parse_dict(std::string& data,int pos);



std::pair<bencodevalue,int> parse_value(std::string& str,int pos){
    if(str[pos]=='i') return parse_int(str,pos+1,'e');
    if(isdigit(str[pos])) return parse_string(str,pos);
    if(str[pos]=='l') return parse_list(str,pos+1);
    if(str[pos]=='d') return parse_dict(str,pos+1);
    throw std::runtime_error("invalid bencode");
}


bencodevalue benvaluedecode(std::string& str){

    auto [value,pos]=parse_value(str,0);

    return value;

}








#endif
