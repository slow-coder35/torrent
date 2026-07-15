#ifndef FILE_IO_H
#define FILE_IO_H
#include<fstream>
#include<ostream>

#include<fcntl.h>
#include<unistd.h>
#include<cstdint>
#include<vector>
//open torrent file and give data 

class torrent_session;

 std::string fetch_torrent_file(const std::string& path);

int create_placeholder_file(torrent_session* t);
void write_to_file(uint32_t piece,torrent_session* t,const std::vector<char>& buffer);


#endif