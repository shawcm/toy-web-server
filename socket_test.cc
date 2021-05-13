#include "socket.h"
#include "eventLoop.h"


int main() {
    EventLoop el;

    TcpSocket s;
    //s.bind(NULL, 12345);
    
    // connect to echo server
    int rc = s.connect("localhost", 7777);
    if (rc == 0) {
        puts("connect returned immediately");
        exit(0);
    } // else

    EventPtr e(new Event(s.fd, EVENT_READ | EVENT_WRITE, &el));
    e->el->addEvent(e);

    e->setHandle(EVENT_TIMEOUT, [e](){
        printf("timeout\n");
        e->el->updateEvent(e);
    });

    // listen() readable: failed
    e->setHandle(EVENT_READ, [e](){
        printf("connect failed\n");
        e->el->deleteEvent(e->fd);
    });

    // liste() writable: success 
    e->setHandle(EVENT_WRITE, [&, e](){
        printf("connect success\n");
        // add new handle to loop
        e->el->deleteEvent(e->fd);

        EventPtr echo(new Event(s.fd, EVENT_WRITE, &el));
        echo->el->addEvent(echo);
        echo->current_size = 0;
        memcpy(echo->buffer, "hello\n", 7); 
        printf("current_size: %d\n", echo->current_size);

        // on writable, write message to server
        debug("set handle write\n");
        echo->setHandle(EVENT_WRITE, [&, echo](){
            debug("echo writable\n");
            printf("current_size: %d\n", echo->current_size);

            int cnt;

            // SEGV?????
            if (7 - (int)(echo->current_size) <= 0)
                cnt = 0;
            else
                cnt = s.write(echo->buffer + echo->current_size, 7 - echo->current_size);

            debug("cnt = %d\n", cnt);

            if (cnt < 0)
                err_exit("write");
            if (cnt == 0) {
                // finish write
                debug("finish write\n");
                echo->eventType = (echo->eventType & ~EVENT_WRITE) | EVENT_READ;
                echo->current_size = 0;

                echo->el->updateEvent(echo);
            } else {
                echo->current_size += cnt;
            }
        });

        // on readable, read message from server
        debug("set handle read\n");
        echo->setHandle(EVENT_READ, [&, echo](){
            debug("echo readable\n");
            int cnt = s.read(echo->buffer + echo->current_size, BUFSIZ - echo->current_size); // TODO: check if current_size == BUFSIZ
            debug("cnt: %d\n", cnt);
            
            switch (cnt) {
                case -1:
                    err_exit("s.read");
                case 0:
                    debug("finished read:\n");
                    echo->buffer[echo->current_size] = '\0'; // in case
                    puts(echo->buffer);
                    exit(0);
                default:
                    echo->current_size += cnt;
            }
        });
    });

    el.loop();
    // char buf[100];
    // int cnt = s.write("hello", 6);
    // s.read(buf, cnt);
    // s.close();
    // puts(buf);

}