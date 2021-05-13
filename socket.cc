#include "socket.h"

Socket::Socket(int fd) : fd(fd) {}


Socket::Socket(int domain, int type, int protocol, bool nonblock) : 
    fd(::socket(domain, type, protocol)),
    domain(domain), type(type), protocol(protocol)
{
    if (fd == -1)   err_exit("socket");

    if (nonblock) {
        int flag = fcntl(fd, F_GETFL);
        flag |= O_NONBLOCK;
        if (fcntl(fd, F_SETFL, flag) == -1)
            err_exit("fcntl");
    }
}


void Socket::bind(const struct sockaddr *addr, socklen_t addrlen) {
    if (::bind(fd, addr, addrlen) == -1)
        err_exit("bind");
}


void Socket::listen() {
    if (::listen(fd, backlog) == -1)
        err_exit("listen");
}


void Socket::connect(const struct sockaddr *addr, socklen_t addrlen) {
    if (::connect(fd, addr, addrlen) == -1)
        err_exit("connect");
}


ssize_t Socket::write(const void *buf, size_t count) {
    return ::write(fd, buf, count);
    // int bytes_writen = 0;
    // while (bytes_writen != count) {
    //     int cnt = ::write(fd, buf+bytes_writen, count - bytes_writen);
    //     if (cnt == -1) {

    //     } else if (cnt == 0) {

    //     } else {

    //     }

    // }
}


ssize_t Socket::read(void *buf, size_t count) {
    return ::read(fd, buf, count);
}


void Socket::close() {
    if (::close(fd) == -1) {
        err_exit("close");
    }
}

Socket::~Socket() {
    // close();
}


TcpSocket::TcpSocket() : Socket(domain, type, protocol) {}


TcpSocket::TcpSocket(int _fd) : Socket(_fd) {
    sockaddr_in peer_addr;
    socklen_t len = sizeof(peer_addr);

    getpeername(fd, (sockaddr*) &peer_addr, &len);
    inet_ntop(AF_INET, &(peer_addr.sin_addr), this->addr, INET_ADDRSTRLEN);
    this->port = ntohs(peer_addr.sin_port);
}


int TcpSocket::connect(const char* addr, uint16_t port) {
    sockaddr_in peer_addr;
    memset(&peer_addr, 0, sizeof(peer_addr));
    socklen_t len = sizeof(peer_addr);

    inet_pton(AF_INET, addr, &(peer_addr.sin_addr.s_addr));
    peer_addr.sin_port = htons(port);
    peer_addr.sin_family = AF_INET;

    if (::connect(fd, (const sockaddr*)&peer_addr, len) == -1)
        return errno;
    return 0;
}


void TcpSocket::bind(const char* addr, uint16_t port) {
    sockaddr_in listen_addr;
    socklen_t len;

    memset(&listen_addr, 0, sizeof(listen_addr));
    inet_pton(AF_INET, addr, &(listen_addr.sin_addr.s_addr));

    listen_addr.sin_port = htons(port);
    listen_addr.sin_family = AF_INET;
    len = sizeof(listen_addr);

    if (::bind(fd, (const sockaddr*)&listen_addr, len) == -1)
        err_exit("bind");

    strncpy(this->addr, addr, INET_ADDRSTRLEN);
    this->port = port;
}


TcpSocket* TcpSocket::accept() {
    debug("TcpSocket::accept");
    sockaddr_in peer_addr;
    socklen_t len = sizeof(peer_addr);

    int client_fd = ::accept(fd, (sockaddr*)&peer_addr, &len);
    if (client_fd == -1)
        return nullptr;

    return new TcpSocket(client_fd);
}


const char* TcpSocket::getAddr() {
    return addr;
}


const uint16_t TcpSocket::getPort() {
    return port;
}