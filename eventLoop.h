#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include "util.h"

// event types
#define EVENT_READ 1
#define EVENT_WRITE 2
#define EVENT_TIMEOUT 4


// minimum epoll wait duration in milliseconds
#define MINIMUM_WAIT 500


class Event;
class EventLoop;

typedef std::shared_ptr<Event> EventPtr;

// typedef void (*onAcceptable)(EventLoop evl, int fd, Event ev);
// typedef void (*onReadable)();
// typedef void (*onWritable)();
// typedef void (*onTimeout)();

typedef std::function<void(void)> EventCallback;


// an event in the event loop
// read, write, timeout events could be registered
class Event {
public:
    Event(EventLoop *el);
    Event(int fd, int eventType, EventLoop *el);
    Event(int fd, int eventType, int nice, EventLoop *el);

    ~Event();

    void setHandle(int eventType, EventCallback cb);

    int fd;
    int eventType; // read / write / timeout OR
    char *buffer;
    size_t current_size;
    size_t capacity; // the maximum size of `buffer`
    time_t timeout; // seconds
    int nice; // -INT_MAX ~ INT_MAX, the bigger, the high priority
    EventLoop *el;

    EventCallback onRead;
    EventCallback onWrite;
    EventCallback onTime;
};


class LightEvent : public Event {

};

class EventLoop {
public:
    EventLoop();
    ~EventLoop();
    
    void addEvent(EventPtr);
    void deleteEvent(int fd);
    void updateEvent(EventPtr);
    void loop();

private:
    int epoll_fd;
    int max_fd; // largest fd
    static const int max_events = 1000; // number of events to look at 
    struct epoll_event evt; // tmp
    struct epoll_event ready_events[max_events]; // modified by epoll_wait
    std::map<time_t, std::unordered_set<int>> clocks; // time -> fd
    std::unordered_map<int, EventPtr> events; // fd -> the events
    std::unordered_map<int, time_t> timeouts;

    void addEvent_aux(EventPtr ev, int epoll_op, bool);
};


void readAll(EventLoop *el, char *buf, size_t len, int _fd, int nice = 0, int timeout = 0);
void writeAll(EventLoop *el, const char *buf, size_t len, int _fd, int nice = 0, int timeout = 0);
void writeAllEvent(EventPtr e, const char *buf, size_t len);

inline void delete_and_close(EventPtr e) {
    e->current_size = 0;
    e->el->deleteEvent(e->fd);
    if (-1 == close(e->fd))
        err_exit("close");
    debug("close(%d)", e->fd);
}

inline void delete_and_close(Event* e) {
    e->current_size = 0;
    e->el->deleteEvent(e->fd);
    if (-1 == close(e->fd))
        err_exit("close");
    debug("close(%d)", e->fd);
}

#endif