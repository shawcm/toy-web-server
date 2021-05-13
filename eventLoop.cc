#include "eventLoop.h"

// default handlers: as a reminder for misuse
struct default_handler {
    int type;     // set the default handler;
    default_handler(int type) : type(type) {}
    void operator()() {
        printf("Misuse: No handler set for %s / %s / %s\n", 
            type & EVENT_READ ? "read" : "",
            type & EVENT_WRITE ? "write" : "",
            type & EVENT_TIMEOUT ? "timeout" : "");
    }
};


Event::Event(EventLoop *el)
    : Event(-1, EVENT_READ, el) {
}


Event::Event(int fd, int eventType, EventLoop *el)
    : Event(fd, eventType, 0, el) {
}


Event::Event(int fd, int eventType, int nice, EventLoop *el)
    : fd(fd), eventType(eventType), nice(nice), el(el) {
        current_size = 0;
        timeout = 0;
        buffer = (char*)malloc(BUFSIZ);
        capacity = BUFSIZ;

        setHandle(EVENT_READ, default_handler(EVENT_READ));     // set the default handler
        setHandle(EVENT_WRITE, default_handler(EVENT_WRITE));     // set the default handler
        setHandle(EVENT_TIMEOUT, default_handler(EVENT_TIMEOUT));     // set the default handler
}


Event::~Event() {
    // printf("event %d get destructed\n", fd);
    if(buffer) free(buffer);
    buffer = NULL;
    // close(fd);
}


void Event::setHandle(int eventType, EventCallback cb) {
    if (eventType & EVENT_READ)
        onRead = cb;
    if (eventType & EVENT_WRITE)
        onWrite = cb;
    if (eventType & EVENT_TIMEOUT)
        onTime = cb;
}


EventLoop::EventLoop() {
    epoll_fd = epoll_create(100);
    signal(SIGPIPE, SIG_IGN);
    // if (-1 == sigaction(SIGPIPE, NULL, &__oact))
    //     err_exit("sigaction");
}


EventLoop::~EventLoop() {

    // sigaction(SIGPIPE, &__oact, NULL);
}


// void EventLoop::addEvent_aux(Event &ev, int epoll_op, bool nonblock = true) {
//     // set file as nonblock
//     if (nonblock) {
//         int mode;
//         mode = fcntl(ev.fd, F_GETFL);
//         if (mode == -1)
//             err_exit("fcntl");

//         mode |= O_NONBLOCK;
//         if (-1 == fcntl(ev.fd, F_SETFL, mode))
//             err_exit("fcntl");
//     }

//     memset(&evt, 0, sizeof(evt));
//     if (ev.eventType & EVENT_READ) evt.events |= EPOLLIN;
//     if (ev.eventType & EVENT_WRITE) evt.events |= EPOLLOUT;
//     if (ev.eventType & EVENT_TIMEOUT) {
//         time_t to = time(NULL) + ev.timeout;
//         timeouts[ev.fd] = to;
//         clocks[to].insert(ev.fd);
//     }
    
//     evt.data.fd = ev.fd;
//     if (epoll_ctl(epoll_fd, epoll_op, ev.fd, &evt) == -1)
//         err_exit("epoll ctl");
    
//     events[ev.fd] = ev;

//     if (ev.fd > max_fd) 
//         max_fd = ev.fd;
// }

void EventLoop::addEvent_aux(EventPtr ev, int epoll_op, bool nonblock = true) {
    // set file as nonblock
    if (nonblock) {
        int mode;
        mode = fcntl(ev->fd, F_GETFL);
        if (mode == -1)
            err_exit("fcntl");

        mode |= O_NONBLOCK;
        if (-1 == fcntl(ev->fd, F_SETFL, mode))
            err_exit("fcntl");
    }

    memset(&evt, 0, sizeof(evt));
    // evt.events |= EPOLLET;
    if (ev->eventType & EVENT_READ) evt.events |= EPOLLIN;
    if (ev->eventType & EVENT_WRITE) evt.events |= EPOLLOUT;
    if (ev->eventType & EVENT_TIMEOUT) {
        time_t to = time(NULL) + ev->timeout;
        timeouts[ev->fd] = to;
        clocks[to].insert(ev->fd);
    }
    
    evt.data.fd = ev->fd;
    if (epoll_ctl(epoll_fd, epoll_op, ev->fd, &evt) == -1)
        err_exit("epoll ctl");
    
    events[ev->fd] = ev;

    if (ev->fd > max_fd) 
        max_fd = ev->fd;
}

void EventLoop::addEvent(EventPtr ev) {
    updateEvent(ev);
}


void EventLoop::updateEvent(EventPtr ev) {
    if (events.count(ev->fd) == 0) {
        // this event not added
        addEvent_aux(ev, EPOLL_CTL_ADD);
        return;
    }

    // clean old data
    time_t to = timeouts[ev->fd];
    if (clocks[to].count(ev->fd)) {
        clocks[to].erase(ev->fd);
        if (clocks[to].empty())
            clocks.erase(to);
    }
    
    addEvent_aux(ev, EPOLL_CTL_MOD);
}


void EventLoop::deleteEvent(int fd) {
    debug("delete event fd %d", fd);

    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1)
        err_exit("epoll_ctl");
    if (fd == max_fd) max_fd--;

    if (events[fd]->eventType & EVENT_TIMEOUT) {
        time_t to = timeouts[fd];
        clocks[to].erase(fd);
        if (clocks[to].empty()) clocks.erase(to);
        timeouts.erase(fd);
    }
    // events[fd]->~Event(); //!!! dangerous, but try
    events.erase(fd);
}


void EventLoop::loop() {
    for (;;) {
    // calculate the next timeout event
        int duration_to_wait = -1;

        if (!clocks.empty()) {
            time_t t = clocks.begin()->first;
            // if some events are timeout, we wait 1000, else
            time_t now = time(NULL);

            if (t <= now) {
                duration_to_wait = MINIMUM_WAIT; 
            } else {
                duration_to_wait = (t - now) * 1000;
            }
        }

        #ifdef do_debug
        // print the eventloop status:
        {
            debug("eventloop status:");
            debug("# %d events:", events.size());
            for (auto &pr : events) {
                int fd = pr.first;
                EventPtr evt = pr.second;
                debug("event fd(%d), buffer(%x), size(%d), capacity(%d), eventType(%d)",
                              fd,     evt->buffer, 
                                                evt->current_size,
                                                            evt->capacity,
                                                                        evt->eventType
                );
            }

            debug("# %d clocks", clocks.size());
            for (auto &pr : clocks) {
                debug("clock = %d", pr.first);
                for (auto fd : pr.second) {
                    debug("  fd = %d", fd);
                }
            }
        }
        #endif

        debug("start wait events");
        int elems_to_wait = epoll_wait(epoll_fd, ready_events, max_events, duration_to_wait);
        debug("# %d ready events", elems_to_wait);

        switch (elems_to_wait) {
            case -1: 
                err_exit("epoll_wait");
            default:
                std::priority_queue<std::pair<int, int>> pq; // nice, index in ready_events
                for (int i = 0; i < elems_to_wait; i++) {
                    int fd = ready_events[i].data.fd;
                    pq.push(std::make_pair(events[fd]->nice, i));
                }

                while (!pq.empty()) {
                    int i = pq.top().second; pq.pop();
                    int fd = ready_events[i].data.fd;
                    uint32_t e = ready_events[i].events;
                    EventPtr ev = events[fd];
                    debug("ready event[%d], fd = %d, nice = %d", i, fd, ev->nice);
                    if (e & EPOLLIN && events[fd]->eventType & EVENT_READ) {
                        ev->onRead();
                    }
                    if (e & EPOLLOUT && events[fd]->eventType & EVENT_WRITE && events.count(fd) != 0) { // in case the event is deleted when read
                        ev->onWrite();
                    }
                }

                // for (int i = 0; i < elems_to_wait; i++) {
                //     int fd = ready_events[i].data.fd;
                //     uint32_t e = ready_events[i].events;
                //     EventPtr ev = events[fd];
                //     debug("ready event[%d], fd = %d", i, fd);
                //     if (e & EPOLLIN) {
                //         ev->onRead();
                //     }
                //     if (e & EPOLLOUT && events.count(fd) != 0) { // in case the event is deleted when read
                //         ev->onWrite();
                //     }
                // }
        }

        // same as before, process timeout events
        if (!clocks.empty()) {
            time_t t = clocks.begin()->first;
            // int   fd = clocks.begin()->second;

            while (t <= time(NULL)) {
                while (clocks.count(t) && !clocks[t].empty()) {
                    int fd = *clocks[t].begin();
                    events[fd]->onTime();
                }
                if (clocks.count(t) && clocks[t].empty()) clocks.erase(t);

                if (clocks.empty()) break;
                t = clocks.begin()->first; 
            }
        }
    }
}



void writeAll(EventLoop *el, const char *buf, size_t len, int _fd, int nice, int timeout) {
    debug("writeAll start");
    int fd = dup(_fd);
    if (fd == -1)
        err_exit("dup");

    int type = EVENT_WRITE;
    if (timeout > 0) type |= EVENT_TIMEOUT;
    EventPtr e(new Event(fd, type, nice, el));
    Event* we = e.get();
    e->el->addEvent(e);

    e->onWrite = [we, buf, len](){
        debug("writeAll onWrite start, fd = %d", we->fd);
        debug("current length = %d\n", we->current_size);
        if (we->current_size >= len) {
            debug("we->current_size == len");
            // finish
            debug("delete_and_close(%d)", we->fd);
            delete_and_close(we);
            return;
        }

        int cnt = write(we->fd, buf + we->current_size, len - we->current_size);
        if (cnt == -1 || cnt == 0) {
            // connect reset by peer
            debug("delete_and_close(%d)", we->fd);
            delete_and_close(we);
            return;
        }

        we->current_size += cnt;
        debug("writeAll onWrite end");
    };

    e->onTime = [we](){
        debug("writeAll timeout begin");
        debug("delete_and_close(%d)", we->fd);
        delete_and_close(we);
        debug("writeAll timeout end");
    };
    debug("writeAll end");
}


void writeAllEvent(EventPtr _e, const char *buf, size_t len) {
    Event *e = _e.get();

    debug("writeAll start");

    if (e->fd == -1)
        err_exit("dup");

    // int type = EVENT_WRITE;

    e->el->addEvent(_e);
    
    e->onWrite = [e, buf, len](){
        debug("writeAll onWrite start, fd = %d", e->fd);
        debug("current length = %d\n", e->current_size);
        if (e->current_size >= len) {
            debug("e->current_size >= len");
            // finish
            debug("delete_and_close(%d)", e->fd);
            delete_and_close(e);
            return;
        }

        int cnt = write(e->fd, buf + e->current_size, len - e->current_size);
        if (cnt == -1 || cnt == 0) {
            // connect reset by peer
            debug("delete_and_close(%d)", e->fd);
            delete_and_close(e);
            return;
        }

        e->current_size += cnt;
        debug("writeAll onWrite end");
    };

    debug("writeAll end");
}


void readAll(EventLoop *el, char *buf, size_t len, int _fd, int nice, int timeout) {
    int fd = dup(_fd);
    if (fd == -1)
        err_exit("dup");

    int type = EVENT_READ;
    if (timeout > 0) type |= EVENT_TIMEOUT;
    EventPtr e(new Event(fd, type, el));
    e->el->addEvent(e);
    Event *we = e.get();

    e->onRead = [we, len, buf](){
        debug("readAll read begin");
        if (we->current_size == len) {
            // finish
            delete_and_close(we);
            return;
        }

        int cnt = write(we->fd, buf + we->current_size, len - we->current_size);

        // if (cnt == -1)
        //     err_exit("write");
        if (cnt == -1 || cnt == 0) {
            debug("not writable");
            debug("delete_and_close(%d)", we->fd);
            delete_and_close(we);
            return;
        }
        we->current_size += cnt;
        debug("readAll read end");
    };

    e->onTime = [we](){
        debug("readAll timeout begin");
        debug("delete_and_close(%d)", we->fd);
        delete_and_close(we);
        debug("readAll timeout end");
    };
}