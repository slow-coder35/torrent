#include"file_io.h"
#include "torrent_session.h"


 std::string fetch_torrent_file(const std::string& path){
    std::ifstream file(path,std::ios::binary);

    if(!file){
        throw std::runtime_error("could not open file\n");
    }
    
    file.seekg(0,std::ios::end);
    size_t size=file.tellg();
    file.seekg(0,std::ios::beg);

    std::string data;
    data.resize(size);
    file.read(data.data(),size);
    file.close();
    return data;

}

 int create_placeholder_file(torrent_session* t){
    auto& metadata=t->metadata;
    if(metadata->multifile());
    int fd = open(metadata->name().c_str(), O_CREAT | O_RDWR, 0644);
    if(fd<0){
        throw std::runtime_error ("could not get file placceholders\n");
    }
    ftruncate64(fd,metadata->total_size());
    return fd;
}



//  void write_to_file(uint32_t piece,torrent_session* t,const std::vector<char>& buffer){
//      uint64_t start_index=piece*t->metadata->piece_length();
//      uint64_t length;
//      std::cout<<"writing file:"<<piece<<"\n";
//      if(piece>t->metadata->total_pieces() || piece<0){
//         throw std::runtime_error("invalid piece");
//         return ;
//      }
//     else if(piece<t->metadata->total_pieces()){
//         //starting_index=piece*piece_size,length of information to be written=piecesize
//         if(t->opfd<0){
//             t->opfd = open(t->metadata->name().c_str(), O_CREAT | O_RDWR, 0644);
//         }
   
//     length=t->metadata->piece_length();
    
//     //return written to connfirm its writing  ignorable
//     }
//     else if(piece==t->metadata->total_pieces()-1){
//         length=buffer.size();
        
//     }
//     size_t written=pwrite(t->opfd,buffer.data(),length,start_index);
//     std::cout<<"file written\n";
//     {
//     std::lock_guard<std::mutex> guard(t->bitfield_lock);
//     t->downloaded_piece_count++;
//     }
// }
