/******************************
*  Github:yichenyiyan  QAQ    *
*******************************/

#include <thread>
#include <functional>

#include "Server.hh"
#include "Socket.hh"
#include "Acceptor.hh"
#include "EventLoop.hh"
#include "ThreadPool.hh"
#include "Connection.hh"
#include "InetAddress.hh"


#define READ_BUFFER 1024


yichen::Server::Server(yichen::EventLoop *_main_Reactor) 
    : main_Reactor(_main_Reactor), acceptor(nullptr) {    
    acceptor = new Acceptor(main_Reactor);
    std::function<void(Socket*)> cb = std::bind(&Server::newConnection, this, std::placeholders::_1);
    acceptor->setNewConnectionCallback(cb);

    int size = std::thread::hardware_concurrency();
    threadPool = new ThreadPool(size);
    for (int i = 0; i < size; ++i) {
        sub_Reactors.push_back(new EventLoop());
    }

    for (int i = 0; i < size; ++i) {
        std::function<void()> sub_loop = std::bind(&EventLoop::loop, sub_Reactors[i]);
        threadPool->add(sub_loop);
    }
}

yichen::Server::~Server() {
    delete acceptor;
    delete threadPool;
}



void 
yichen::Server::newConnection(Socket* sock) {
    if (sock->m_getFd() != -1) {
        int random = sock->m_getFd() % sub_Reactors.size();
        Connection* conn = new Connection(sub_Reactors[random], sock);
        std::function<void(int)> cb = std::bind(&yichen::Server::deleteConnection, this, std::placeholders::_1);
        conn->setDeleteConnectionCallback(cb);
        connections[sock->m_getFd()] = conn;
    }
}

void 
yichen::Server::deleteConnection(int sockfd) {
    if (sockfd != -1) {
        auto it = connections.find(sockfd);
        if (it != connections.end()) {
            Connection *conn = connections[sockfd];
            connections.erase(sockfd);
            // close(sockfd);       //正常
            delete conn;         //会Segmant fault
        }
    }
}

