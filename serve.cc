#include "serve.h"
#include "file_system.h"

HttpListener::HttpListener(EventLoop *el) : HttpListener(el, HTTP_PORT, "/var/www/") {}


HttpListener::HttpListener(EventLoop *el, uint16_t port, string root_dir)
    : port(port), root_dir(root_dir), event(new Event(el)), sock()
{
    sock.bind("localhost", port);
    sock.listen();
}


void HttpListener::settle() {
    debug("inside listener settle\n");
    event->fd = sock.fd;
    event->eventType = EVENT_READ;
    event->el->addEvent(event);
    event->nice = 10;
    debug("event fd = %d, event read\n", sock.fd);
    // listen sock readable, accept new connection
    EventCallback onRead = [this]() {
        debug("event %d read execute\n", event->fd);
        while (true) {
            TcpSocket::Ptr s(sock.accept());
            if (s == nullptr)
                break;
            HttpConnection conn(s, event->el, this);
            debug("new connection: fd(%d)", s->fd);
            conn.settle();
            debug("event %d read leave\n", event->fd);
        } // conn is destructed!
    }; 

    event->setHandle(EVENT_READ, onRead);
    debug("leave listener settle\n");
}


HttpConnection::HttpConnection(TcpSocket::Ptr sock, EventLoop *el, HttpListener *listener) : 
    sock(sock), 
    event(new Event(sock->fd, EVENT_READ | EVENT_TIMEOUT, el)),
    builder(event->buffer),
    num_unhandled(0),
    listener(listener),
    seq_number(0) {
    
    event->timeout = 10;
    // debug("http connection event fd = %d created\n", event->fd);
}

HttpConnection::HttpConnection(TcpSocket::Ptr sock, EventLoop *el) : 
    HttpConnection(sock, el, NULL) {
    
    event->timeout = 10;
    // debug("http connection event fd = %d created\n", event->fd);
}


// construct the response
inline void proposeReplyQuick(HttpResponse &reply, HttpStatus s) {
    reply.status = s;
    reply.version = "HTTP/1.1";
    reply.header.Content_Length = "0";
    reply.content = NULL;
    reply.content_len_ = 0;
}


// construct the response
// inline void proposePageReply(PageWriter &pw, HttpResponse &reply, HttpStatus s, const char* fname) {
//     reply.status = s;
//     reply.version = "HTTP/1.1";
//     reply.header.Content_Length = "0";
//     reply.content = NULL;
//     reply.content_len_ = 0;

//     pw.write(fname, e->buffer + len, BUFSIZ - len);
// }


void HttpConnection::settle() {
    // event->timeout = CLIENT_TIMEOUT;

    // readable: 
    // 1. read request header...
    // 2. prepare reply
    // 3. set event writable

    // BUG: 不能响应 keep-alive 使用一个连接多次传输数据
    debug("make duplicated object\n");
    std::shared_ptr<HttpConnection> conn(new HttpConnection(*this));
    
    event->setHandle(EVENT_READ, [conn](){
        debug("inside connection EVENT_READ\n");
        if (conn->event->capacity - conn->event->current_size <= 0) {
            debug("header is larger than buffer");
            // header is larger than buffer

            proposeReplyQuick(conn->reply, BadRequest);
            ssize_t len = conn->reply.dump(conn->event->buffer, BUFSIZ);
            // TODO: ld we handle timeout in writeALL?
            writeAll(conn->event->el, conn->event->buffer, len, conn->event->fd);
            delete_and_close(conn->event);
            return;
        }

        int cnt = conn->sock->read(conn->event->buffer + conn->event->current_size, conn->event->capacity - conn->event->current_size);
        debug("conn %d event buffer: %s", conn->event->fd, conn->event->buffer);
        if (cnt == -1) {
            // socket read -1, connection reset
            debug("socket read -1, connection reset");
            delete_and_close(conn->event);
            return;
        } else if (cnt == 0) {
            debug("connection fd=%d read 0, connection closed ", conn->event->fd);
            // connection closed
            delete_and_close(conn->event);
            return;
        }
        conn->event->current_size += cnt;
        debug("conn %d event buffer size %d", conn->event->fd, conn->event->current_size);
        while (true) {
            if (cnt <= 0) break;
            int builder_status_code = conn->builder.progress(cnt);
            debug("build status %d", builder_status_code);
            if (builder_status_code == 1) {
                // finish read client request,
                debug("build completes");
                conn->builder.reset();
                cnt = conn->builder.current_len;
                conn->request.construct(conn->event->buffer, conn->event->current_size);
                conn->event->current_size = cnt;
                // conn->num_unhandled++;    // one more unhandled connection
                conn->seq_number++;
                EventPtr e(new Event(dup(conn->sock->fd), EVENT_WRITE, -conn->seq_number, conn->event->el));

                const char *fname = conn->request.location.c_str();
                string full_path = FileSystem::getFullPath(conn->listener->root_dir.c_str(), fname);
                debug("full path: %s\n", full_path.c_str());

                PageWriter pw(conn->listener->root_dir.c_str());
                // create a new event to reply the request
                // put reply to e->buffer, then set write handle to e, add e to eventloop
                HttpResponse::Ptr resonse(new HttpResponse());
                resonse->version = "HTTP/1.1";
                

                if (!FileSystem::fileExists(full_path.c_str())) {
                    resonse->status = NotFound;
                    resonse->header.Content_Type = "text/html; charset=utf-8";
                    fname = "404.html";
                    full_path = FileSystem::getFullPath(conn->listener->root_dir.c_str(), fname);
                    // resonse->header.Content_Length = "0";
                    // resonse->header.Connection = "keep-alive";

                } else {
                    resonse->status = OK;
                    // resonse->header.Connection = "keep-alive";
                    // resonse->header.Content_Length = std::to_string(FileSystem::fileSize(full_path.c_str()));
                }
                resonse->header.Connection = "keep-alive";
                resonse->header.Content_Length = std::to_string(FileSystem::fileSize(full_path.c_str()));

                ssize_t len = resonse->dump(e->buffer, BUFSIZ);
                if (len + FileSystem::fileSize(full_path.c_str()) > BUFSIZ) {
                    e->buffer = (char*)realloc(e->buffer, len + FileSystem::fileSize(full_path.c_str()));
                }

                if (FileSystem::fileExists(full_path.c_str())) {
                    pw.write(fname, e->buffer + len, FileSystem::fileSize(full_path.c_str()));
                }

                // e->current_size = 0;
                // std::min((size_t) BUFSIZ, len + FileSystem::fileSize(full_path.c_str()));

                writeAllEvent(e, e->buffer, len + FileSystem::fileSize(full_path.c_str()));

                if (conn->request.header.Connection == "close") {
                    delete_and_close(e);
                } else {
                    // conn->event->eventType |= EVENT_WRITE;
                    conn->event->el->updateEvent(conn->event);
                }
            } else if (builder_status_code == 0){
                conn->event->el->updateEvent(conn->event); // update the timeout
                cnt = 0;
            } else {
                // error parsing
                // create a new event to reply the request
                conn->event->current_size = 0;
                EventPtr e(new Event(dup(conn->sock->fd), EVENT_WRITE, -conn->seq_number, conn->event->el));
                // put reply to e->buffer, then set write handle to e, add e to eventloop
                HttpResponse::Ptr resonse(new HttpResponse());
                proposeReplyQuick(*resonse, BadRequest);
                ssize_t len = resonse->dump(e->buffer, BUFSIZ);
                writeAllEvent(e, e->buffer, len);
                delete_and_close(conn->event);
                cnt = 0;
            }
        }
        debug("leaving event_read");
    });

    // // write reply
    // // if write finish, disable write event
    // event->setHandle(EVENT_WRITE, [conn](){
    //     debug("inside connection EVENT_write\n");
        
    //     proposeReplyQuick(conn->reply, OK);
    //     ssize_t len = conn->reply.dump(conn->event->buffer, BUFSIZ);
    //     conn->event->el->updateEvent(conn->event);
    //     writeAll(conn->event->el, conn->event->buffer, len, conn->event->fd, -conn->seq_number);

    //     if (conn->num_unhandled <= 0) err_exit("error, unhandled should not be 0"); // not expected to happen
    //     if (--conn->num_unhandled == 0) {
    //         debug("set conn as not writable");
    //         conn->event->eventType &= ~EVENT_WRITE;
    //     }

    //     debug("leaving connection EVENT_write\n");
    // });

    // timeout
    // remove event
    // send reply of 408 Request Timeout
    event->setHandle(EVENT_TIMEOUT, [conn](){
        debug("inside connection EVENT_timeout");

        // proposeReplyQuick(conn->reply, RequestTimeout);
        // ssize_t len = conn->reply.dump(conn->event->buffer, BUFSIZ);
        // writeAll(conn->event->el, conn->event->buffer, len, conn->event->fd);
        delete_and_close(conn->event);
        debug("leaving connection EVENT_timeout\n");
    });

    debug("add event\n");
    event->el->addEvent(event);
    debug("finish connection settle()\n");
}


Server::Server(uint16_t port, string root_dir) :
    port(port),
    root_dir(root_dir),
    listener(&el, port, root_dir)
{
    pid = getpid();
    printf("----- Http Server -----\n"
          "  pid  = %d\n"
          "  port = %u\n"  
          "  root = %s\n"
          "-----------------------\n",
          pid,
          port,
          root_dir.c_str());
}

void Server::serve() {
    listener.settle();
    el.loop();
}