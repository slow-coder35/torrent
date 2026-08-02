#ifndef TORRENTSESSION_H
#define TORRENTSESSION_H
#define BLOCK_LENGTH 4096
#define MAX_CONNECTIONS_PER_THREAD 50


#include "torrent.h"
#include "peerconnection.h"
#include "tracker_client.h"
#include"peer_info.h"
#include <mutex>
#include <list>
#include"piecemanager.h"
#include <thread>
#include<sys/epoll.h>
#include<array>
#include<map>
#include<memory>
#include<unordered_map>



#include"worker.h"

// struct  piece{

//     uint32_t id;
    
//     bool to_download{true};
//     bool downloaded;
//     bool downloading;
//     bool verifying;

// };

#include "file_io.h"
class bit_f;
class torrent_session;




class torrent_session{
    
    
    
    public:
        torrent_session(std::shared_ptr<torrent> metadata):metadata(metadata),piece_manager(metadata->total_pieces()){
            t=this;
            peer_id=generate_binary_peer_id();
            client=trackerclient(metadata,peer_id);
            // mbitfield.bitfield.resize(metadata->total_pieces());
            filemanager=file_manager(metadata->file_list(),this);
        }



        uint32_t downloaded_num{0};  //can change logic for it when i add pause stop force start maybe a function to get the count when required or sstarting a new seession
        
        std::atomic<bool> torrent_complete=false;
        piecemanager piece_manager;

        // bit_f mbitfield;
        std::mutex bitfield_lock;    //rename to active pieces others are not needed anymore 

        std::atomic<uint32_t> downloaded_piece_count{0};
        
        std::map <int,activepiece> active_pieces;
        std::shared_ptr<torrent> metadata;
        std::string peer_id;
        file_manager filemanager;

    //pass it to torrent header for processing and giving out metadata  //donr
        

    //pass i tti tracker_client where the get req is sent  //done
    ////should be handeld by downloadeder or one more level up not sure  //done 

    //write a function to obtain peer_connection and handshake in this file itself


    

    void start(){
        get_clients();
        epoll_fd=epoll_create1(0);
        worker0.start();
        worker1.start();

        get_connections1();
   
    }


    // void keep_downloading(){
    //     while(!t->torrent_complete){
    //     }
        
    // }



    // int get_connections(){
    //     int i=0;
    //     for (auto p : client.peer_list){
    //         std::cout <<"ip:"<<p.ip<<'\n' <<"\n";   //log lines nothing of value
    //         peerconnection temp(p,metadata,t);
    //         if(temp.connect()) {peer_connections.push_back(std::move(temp));i++;
    //             std::cout<<" is connected\n";
    //         // temp.communication();
    //         }

    //     }
    //     connections=i;
    //     return i;
    // }

    int get_connections1(){
        //make it 100 at a time
    
        std::vector<epoll_event> events;
        events.resize(100);
        int client_idx{0};

        while(!t->torrent_complete){


        for(int i=client_idx;i<client.peer_list.size() && i < client_idx+100 ;i++){
            auto p = std::make_unique<peerconnection>(client.peer_list[i], metadata, t);
            p->connect();




            


            epoll_event ev{};
            if(p->status==ConnectStatus::FAILED){
                continue;
            }
            else if(p->status==ConnectStatus::IN_PROGRESS){
                ev.events=EPOLLOUT;
            }
            else{
            p->send_handshake(t->peer_id);
            ev.events=EPOLLIN;
            }
            
            ev.data.fd=p->sockfd();

            //add to my watch list and create a array for epoll aswell 
            epoll_ctl(epoll_fd,EPOLL_CTL_ADD,p->sockfd(),&ev);
            peer_connections.emplace(p->sockfd(),std::move(p));

        }
        client_idx+=std::min(100,static_cast<int>(client.peer_list.size())-client_idx+1);
        // std::array<epoll_event,100> events;   //for now lets go with vectors
        
        //vector for now


        int n=epoll_wait(epoll_fd,events.data(),events.size(),-1);

        for(int i=0;i<n;i++){
            int fd=events[i].data.fd;        //socketfd
            auto it = peer_connections.find(fd);
            if(it==peer_connections.end()) continue;
            if(it->second->status==ConnectStatus::IN_PROGRESS){
                int error=0;
                socklen_t len=sizeof(error);
                if(getsockopt(fd,SOL_SOCKET,SO_ERROR,&error,&len)<0 || error!=0){
                    //getsockopt failed
                    peer_connections.erase(it);
                    continue;
                }
            
                //succeded atp now add with epollin

                it->second->status=ConnectStatus::CONNECTED;
                it->second->send_handshake(t->peer_id);
                epoll_event ev{};
                ev.data.fd=it->second->sockfd();
                ev.events=EPOLLIN;
                epoll_ctl(epoll_fd,EPOLL_CTL_MOD,fd,&ev);
                continue;
                
            }


            if(it!=peer_connections.end() && it->second->recieve_handshake()){
                auto peer=std::move(it->second);
                peer_connections.erase(it);

                command current;
                current.type = CommandType::add;
                current.peer_connection = std::move(peer);

                if(worker0.count()==worker1.count() && worker1.count()==MAX_CONNECTIONS_PER_THREAD){
                    //prune some inactive threads 
                }

                if(worker0.count()<=worker1.count()  ){
                    worker0.add_to_queue(std::move(current));
                }
                else{
                    worker1.add_to_queue(std::move(current));
                }
            }
        }
    }

}




    // void start_communication(){                            //till handshake
    //     for (auto& connection:peer_connections){
    //         threads.emplace_back(&peerconnection::communication,&connection);
    //         std::cout << "communication started with "<< connection.pinfo().ip<<'\n';
    //     }
    // }

    void wait_to_finish(){        
        
        worker0.stop();
        worker1.stop();
        worker0.join();
        worker1.join();
    }

    void get_clients(){
        client.get_peer_list();
    }

    //iterate the vector add features so that it can download and upload stuff 
    //use network.h and downloadmanager.h to comlete the task 
    //now pass the buffer once over to write file from peerconnection for every request 

    
    
    private:
    
    std::unordered_map<int,std::unique_ptr<peerconnection>> peer_connections;
    trackerclient client;
    torrent_session *t;
    int connections;
    std::list<std::thread> threads;
    worker worker0,worker1;
    int count0{0},count1{0};
    int epoll_fd;
    int total_downloaded_pieces{0};


    int next_client_index=0;




  


};













#endif