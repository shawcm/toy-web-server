#include "eventLoop.h"

void onFileReadable(EventPtr ev) {
    if (ev->current_size == BUFSIZ) {
        printf("buffer full\n");
        return;
    }

    int cnt = read(ev->fd, ev->buffer + ev->current_size, std::min(BUFSIZ - ev->current_size, (unsigned long)24));
    if (cnt == -1) err_exit("read");

    ev->current_size += cnt;
    if (cnt == 0 || ev->current_size == BUFSIZ) {
        printf("done reading:\n");
        ev->buffer[BUFSIZ-1] = '\0';
        puts(ev->buffer);
        ev->el->deleteEvent(ev->fd); // put before close();
        // close(ev->fd); // close() auto delete 
        if (-1 == close(ev->fd))
            err_exit("close");
    }
}


void onFileWritable(EventPtr ev) {
    char buf[1024];
    time_t ticks = time(NULL);
    snprintf(buf, 1023, "%s\n", ctime(&ticks));
    int cnt = write(ev->fd, buf, strlen(buf));
    printf("---writer: wroter %d\n", cnt);
    if (cnt == -1) err_exit("write");
}


void onTimeout(EventPtr ev) {
    char buf[1024];
    time_t ticks = time(NULL);
    snprintf(buf, 1023, "%s\n", ctime(&ticks));
    printf("---timeout %s\n", buf);
    ev->el->updateEvent(ev);
}


int main() {
    EventLoop el;
    int fd = open("in.txt", O_RDONLY);
    int fd2 = open("out.txt", O_WRONLY);

    if (fd == -1 || fd2 == -1) err_exit("read");
    
    EventPtr p(new Event(fd, EVENT_READ, &el));
    EventPtr p2(new Event(fd2, EVENT_WRITE | EVENT_TIMEOUT, &el));
    p2->timeout = 2;

    p->setHandle(EVENT_READ, std::bind(onFileReadable, p));
    p2->setHandle(EVENT_WRITE, std::bind(onFileWritable, p2));
    p2->setHandle(EVENT_TIMEOUT, std::bind(onTimeout, p2));

    el.addEvent(p);
    el.addEvent(p2);

    el.loop();
}