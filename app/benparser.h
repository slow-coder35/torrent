#ifndef BENPARSER_H
#define BENPARSER_H


#include <string>
#include <vector>
#include <map> 
#include <memory>
#include <variant>
#include <cctype>
#include<stdexcept>



#include<fstream>
#include<sstream>
#include<string>

struct info_start_end{
    int start{-1};
    int end{-1};
};

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
std::pair<bencodevalue,int> parse_int(const std::string& data,int pos,char c)
{
    
    bencodeint number={0};
    bool isnegative=(data[pos]=='-');
    if(isnegative) pos++;
    int idx=pos;
    if(data[pos]=='0' && isnegative) throw std::runtime_error("invalid bencode 1");
    while(data[pos]!=c && isdigit(data[pos])){
        number=number*10+ (data[pos]-'0');
        pos++;
    }
    if((!isdigit(data[pos]) && data[pos]!=c)|| idx==pos ) throw std::runtime_error("invalid bencode 2");
    if(isnegative) number=-1*number;
    bencodevalue result;
    result.value=number;
    return {result,pos+1};
}


std::pair<bencodevalue,int> parse_string(const std::string& data,int pos){
    bencodestring ret{""};
    
    auto [n,x]=parse_int(data,pos,':');
    pos=x;
    bencodeint m = std::get<bencodeint>(n.value);
    for(int i=0;i<m;i++){
        ret=ret+data[pos+i];
    }
    pos=pos+m;
    bencodevalue result;
    result.value=ret;
    return {result ,pos};

}




std::pair<bencodevalue,int> parse_value(const std::string& data,int pos,info_start_end* i=nullptr);

std::pair<bencodevalue,int> parse_list(const std::string& data,int pos,info_start_end* i=nullptr){
    bencodevalue temp;
    bencodelist ret;
    
    
  while(data[pos]!='e'){
    auto [x,y] = parse_value(data,pos,i);
    ret.push_back(x);
    pos=y;
  }
    // if(data[pos]!='e') throw std::runtime_error("wrong bencode");
    temp.value=ret;
    return {temp,pos+1} ;


}


std::pair<bencodevalue,int> parse_dict(const std::string& data,int pos,info_start_end* i=nullptr){
    
    bencodedict dict;
    bencodevalue ret;
    // if(data[pos]=='e') throw std::runtime_error("invalid bencode");   //empty dict are perfectly valid
    int idx=pos;
    while(data[pos]!='e'){    
        //get key{always a string btw btw} 
        if(!isdigit(data[pos])) throw std::runtime_error("wrong format benvode");
        auto [x,y]=parse_string(data,pos);
        std::string key=std::get<bencodestring>(x.value);
        if(key=="info"){
            //fill out the info_Start_end for sha-1 hash generation
            i->start=y;    
        }
        pos=y;
        //get value {bencodevalue is how we take back the values}
        auto [a,b]=parse_value(data,y,i);
        dict[key]=a;
        pos=b;
        if(key=="info"){
            i->end=b;
        }       
    }
    // if(idx==pos) throw std::runtime_error(" wrong format");
    ret.value=dict;
    return {ret,pos+1};
}



std::pair<bencodevalue,int> parse_value(const std::string& str,int pos,info_start_end* i=nullptr){
    if(str[pos]=='i') return parse_int(str,pos+1,'e');
    if(isdigit(str[pos])) return parse_string(str,pos);
    if(str[pos]=='l') return parse_list(str,pos+1);
    if(str[pos]=='d') return parse_dict(str,pos+1);
    throw std::runtime_error("invalid bencode");
}



   

bencodevalue benvaluedecode(const std::string& str,info_start_end* i=nullptr){
    auto [value,pos]=parse_value(str,0);
    return value;

}








#endif
