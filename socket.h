#ifndef SOCKET_H
#define SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "util.h"


class Socket {
public:
    typedef std::shared_ptr<Socket> Ptr;

    Socket(int fd);
    Socket(int domain, int type, int protocol, bool nonblock = true);
    ~Socket();

    int fd;
    void bind(const struct sockaddr *addr, socklen_t addrlen);
    void connect(const struct sockaddr *addr, socklen_t addrlen);
    void listen();
    ssize_t write(const void *buf, size_t count);
    ssize_t read(void *buf, size_t count);
    virtual Socket* accept() = 0;
private:
    void close();

private:
    int domain;
    int type;
    int protocol;
    bool nonblock;
    char *addr;
    uint16_t port;
    static const int backlog = 1000;
};


class TcpSocket : public Socket {
public:
    typedef std::shared_ptr<TcpSocket> Ptr;

    TcpSocket();
    TcpSocket(int fd);
    void bind(const char* addr, uint16_t port);

    // connect() return errno on error, 
    // a non-blocking socket is likely to error,
    // the errno maybe EINPROGRESS, EALREADY
    // user should check the readable/writable event of the socket
    int connect(const char* addr, uint16_t port);
    const char* getAddr();
    const uint16_t getPort();
    virtual TcpSocket* accept() override;

private:
    static const int domain = AF_INET;
    static const int type = SOCK_STREAM;
    static const int protocol = 0;
    char addr[INET_ADDRSTRLEN];
    uint16_t port;
};

#endif