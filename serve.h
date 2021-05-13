#ifndef SERVE_H
#define SERVE_H

#include "util.h"
#include "eventLoop.h"
#include "http.h"
#include "socket.h"

#define HTTP_PORT 2377
#define CLIENT_TIMEOUT 10 // 10 seconds before timeout

class HttpListener {
public:
    // listener, fd is the listenning socket fd, el is the main event loop
    typedef std::shared_ptr<HttpListener> Ptr;

    HttpListener(EventLoop *el, uint16_t port, string root_dir);
    HttpListener(EventLoop *el);
    int port;
    string root_dir;
    EventPtr event;
    TcpSocket sock;
    void settle();
};

// HttpConnection::HttpConnection(const HttpConnection &conn) :
//  HttpConnection() {

// }

class HttpConnection {
public:
    typedef std::shared_ptr<HttpConnection> Ptr;
    HttpConnection(TcpSocket::Ptr sock, EventLoop *el, HttpListener *listener);
    HttpConnection(TcpSocket::Ptr sock, EventLoop *el);
    // HttpConnection(const HttpConnection &conn);


    TcpSocket::Ptr sock;
    EventPtr event;
    HttpRequestProgressBuilder builder;
    size_t num_unhandled;
    HttpListener *listener;
    size_t seq_number; // 同时到达的多个请求的编号，递增

    HttpRequest request;
    HttpResponse reply;
    
    // std::queue<EventPtr> event;
    // std::queue<HttpRequest::Ptr> request;
    // std::queue<HttpResponse::Ptr> reply;
    // std::queue<HttpRequestProgressBuilder::Ptr> builder;
    
    void settle(); // set the event handlers
    void sendReply();
    void sendReply(const char* content);
    void sendPage(const char *location);
};


class Server {
public:
    typedef std::shared_ptr<Server> Ptr;
    Server(uint16_t port, string root_dir);
    void serve();

private:
    pid_t pid;
    int port;
    string root_dir;
    
    EventLoop el;
    HttpListener listener;
};


#endif