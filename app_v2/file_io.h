#ifndef FILE_IO_H
#define FILE_IO_H
#include <fstream>
#include <ostream>

#include <fcntl.h>
#include <unistd.h>
#include <cstdint>
#include <vector>

#include "misc.h"
// open torrent file and give data

class torrent_session;

class file_manager
{
public:
    file_manager() {}
    file_manager(const std::vector<file> &fileslist, torrent_session *t) : files(fileslist), t(t)
    {
        for (auto &f : files)
        {
            f.create_file();
        }
    }

    int write_piece(uint32_t piece, const std::vector<char> &buffer);
    
private:
    std::vector<file> files;
    torrent_session *t;
};






std::string fetch_torrent_file(const std::string &path);

int create_placeholder_file(torrent_session *t);
void write_to_file(uint32_t piece, torrent_session *t, const std::vector<char> &buffer);

#endif