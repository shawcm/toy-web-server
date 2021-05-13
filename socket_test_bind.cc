#include "socket.h"
#include "eventLoop.h"


int main() {
    EventLoop el;

    TcpSocket s;
    s.bind("localhost", 7777);
    s.listen();
    // connect to echo server

    EventPtr e(new Event(s.fd, EVENT_READ, &el));
    e->el->addEvent(e);
    
    // listen() readable: accept new connections
    e->setHandle(EVENT_READ, [&, e](){
        TcpSocket::Ptr conn(s.accept());
        debug("conn : fd(%d), %s:%d\n", conn->fd, conn->getAddr(), conn->getPort());

        // add echo server handler for conn
        EventPtr echo(new Event(conn->fd, EVENT_READ | EVENT_WRITE, &el));
        echo->current_size = 0;

        ssize_t *current_writen = (ssize_t *)echo->buffer;
        *current_writen = 0;

        echo->buffer += sizeof(ssize_t);

        echo->el->addEvent(echo);

        echo->setHandle(EVENT_READ, [&, conn, echo, current_writen](){
            // 1. read client message
            if (echo->current_size >= BUFSIZ) {
                printf("buffer full\n");
                return;
            }

            int cnt = conn->read(echo->buffer + echo->current_size, BUFSIZ - echo->current_size);
            switch (cnt) {
                case -1:
                    err_exit("read -1");
                case 0:
                    // client closed
                    printf("client reset %s:%d\n", conn->getAddr(), conn->getPort());
                    // remove event echo
                    echo->el->deleteEvent(echo->fd);
                    conn->close();
                    return;
                default:
                    echo->current_size += cnt;
            }
        });

        echo->setHandle(EVENT_WRITE, [&, conn, echo, current_writen](){
            // 2. send reply to client
            if (*current_writen >= echo->current_size) return;

            int cnt = conn->write(echo->buffer + *current_writen, echo->current_size - *current_writen);
            
            switch (cnt) {
                case -1:
                    err_exit("write -1");
                case 0:
                    // client closed
                    printf("client reset %s:%d\n", conn->getAddr(), conn->getPort());
                    // remove event echo
                    echo->el->deleteEvent(echo->fd);
                    conn->close();
                    return;
                default:
                    *current_writen += cnt;
            }
        });
    });


    el.loop();

}