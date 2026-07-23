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

    int write_piece(uint32_t piece, const std::vector<char> &buffer)
    {
        uint64_t piece_begin = static_cast<uint64_t>(piece) * t->metadata->piece_length();
        uint64_t written = 0;

        for (auto &f : files)
        {
            uint64_t current = piece_begin + written;

            // file is before current write position
            if (current >= f.end)
                continue;

            // nothing left to write
            if (written >= buffer.size())
                break;

            uint64_t file_offset = current - f.begin;

            uint64_t remaining_file = f.size - file_offset;
            uint64_t remaining_piece = buffer.size() - written;

            uint64_t length = std::min(remaining_file, remaining_piece);

            size_t ret=f.write(buffer.data()+written,length,file_offset);
            if(ret!=static_cast<size_t>(length)) return -1;

            written += length;
        }

        return (written == buffer.size()) ? 0 : -1;
    }

private:
    std::vector<file> files;
    torrent_session *t;
};






std::string fetch_torrent_file(const std::string &path);

int create_placeholder_file(torrent_session *t);
void write_to_file(uint32_t piece, torrent_session *t, const std::vector<char> &buffer);

#endif