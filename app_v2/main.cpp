#include "torrent_client.h"
#include "torrent_session.h"

#include <string>
#include<iostream>
#include<stdlib.h>


int main(int argc,char* argv[]){

    if(argc!=3) return -1;
    torrent_client client; 
    std::string command=argv[1];
    if(command=="download"){
        std::string path=argv[2];
        client.download(path);
    }

return 0;

}