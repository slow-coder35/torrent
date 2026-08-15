#pragma once

#define MAX_EVENTS 50

#include <memory>
#include <sys/epoll.h>
#include <thread>
#include <array>
#include <map>
#include <unordered_map>
#include "peerconnection.h"
#include <queue>
#include "misc.h"
#include <mutex>

struct command
{

    CommandType type;
    std::unique_ptr<peerconnection> peer_connection;
};

class worker
{
public:
    worker()
    {
        epoll_fd = epoll_create1(0);
    }
    ~worker()
    {
        close(epoll_fd);
        stop();
    }

    int count()
    {
        return connected_peers.size();
    }



    void run_worker()
    {

        while (running)
        {
            process_commands();

            std::array<epoll_event, MAX_EVENTS> events;
            int n = epoll_wait(epoll_fd, events.data(), events.size(), 1);

            for (int i = 0; i < n; i++)
            {
                int sock_fd = events[i].data.fd;
                auto it = connected_peers.find(sock_fd);
                
                if(it==connected_peers.end()) {
                    // a poissibility rn have to figure it out
                    continue;
                }

                if(events[i].events & EPOLLOUT){
                    it->second->flush_send_buffer();
                }

                if(events[i].events & EPOLLIN){
                it->second->on_recv();
                }

            }
        }
    }


    void start()
    {
        thread_ = std::thread(&worker::run_worker, this);
    }

    void join()
    {
        thread_.join();
    }

    void stop()
    {
        running = false;
    }

    void add_to_queue(command current)
    {   
        std::lock_guard<std::mutex> guard(commands_mtx);
        commands.push(std::move(current));
    }

private:
    std::unordered_map<int, std::unique_ptr<peerconnection>> connected_peers; // sock_fd vs peerconnection objects
    std::queue<command> commands;
    std::mutex  commands_mtx;
    std::thread thread_;
    int epoll_fd{-1};

    bool running{true};

    void process_commands()
    {   
        
        while (!commands.empty())
        {   

            auto current = std::move(commands.front());
            
            switch (current.type)
            {
            case (CommandType::add):
            {
                // std::cout<<" peer recieved in worker\n";
                
                epoll_event ev{};
                ev.events = EPOLLIN | EPOLLOUT;
                ev.data.fd = current.peer_connection->sockfd();
                

                int ret=epoll_ctl(epoll_fd, EPOLL_CTL_ADD, current.peer_connection->sockfd(), &ev);
                if(ret==-1) perror("epoll_ctl\n");
                connected_peers.insert({current.peer_connection->sockfd(), std::move(current.peer_connection)});
                break;
            }

            case (CommandType::disconnect):
            {

                break;
            }
            case (CommandType::shutdown_worker):
            {   
                stop();
                break;
            }
            }
            commands.pop();
        }
    }
};
