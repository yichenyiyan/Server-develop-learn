/**
 * @file Epoll.cc
 * @author yichenyan (2513626958@qq.com)
 * @brief
 * @version 1.0
 * @date 2024-01-01
 * @github https://github.com/yichenyiyan
 * @copyright Copyright (严亦辰) 2024
 *
 */



#include "../include/Epoll.hh"
#include "../include/Channel.hh"

#include <unistd.h>


#define MAX_EVENTS 1024

yichen::Epoll::Epoll() 
    : epfd(-1), events(nullptr) {
    epfd = epoll_create1(0);
    DEBUG(epfd == -1, "epoll create error");
    events = new epoll_event[MAX_EVENTS];
    bzero(events, sizeof(*events) * MAX_EVENTS);
}

yichen::Epoll::~Epoll() {
    if (epfd != -1) {
        close(epfd);
        epfd = -1;
    }
    delete [] events;
}


std::vector<yichen::Channel*> 
yichen::Epoll::poll(int timeout) {
    std::vector<Channel*> activeChannels;
    int nfds = epoll_wait(epfd, events, MAX_EVENTS, timeout);
    DEBUG(nfds == -1, "epoll wait error");
    for (int i = 0; i < nfds; ++i) {
        Channel *ch = (Channel*)events[i].data.ptr;
        ch->m_setReady(events[i].events);
        activeChannels.push_back(ch);
    }
    return activeChannels;
}


void 
yichen::Epoll::updateChannel(Channel *channel) {
    int fd = channel->m_getFd();
    struct epoll_event ev;
    bzero(&ev, sizeof(ev));
    ev.data.ptr = channel;
    ev.events = channel->m_getEvents();
    if (!channel->m_getInEpoll()) {
        DEBUG(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1, "epoll add error");
        channel->m_setInEpoll();
    } 
    else {
        DEBUG(epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1, "epoll modify error");
    }
}

void 
yichen::Epoll::deleteChannel(Channel *channel){
    int fd = channel->m_getFd();
    DEBUG(epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL) == -1, "epoll delete error");
    channel->m_setInEpoll(false);
}