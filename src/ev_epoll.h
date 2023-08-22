#include <fcntl.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>

#define EV_MAX_EVENTS 10

typedef enum EventKind {
  EVENT_READ,
  EVENT_WRITE,
  EVENT_CLOSE,
} EventKind;

typedef struct Event {
  EventKind kind;
  void *data;
} Event;

typedef struct EPollEventLoop {
  int epfd;
} EPollEventLoop;

typedef struct EPollEventHandle {
  int epfd;
  int index;
  struct epoll_event events[EV_MAX_EVENTS];
} EPollEventHandle;

int ev_create(EPollEventLoop *ev) {
  ev->epfd = epoll_create(1);
  if (ev->epfd == -1) {
    perror("epoll_create");
    return 1;
  }
  return 0;
}

int ev_get_handle(EPollEventLoop *ev, EPollEventHandle *handle) {
  handle->epfd = ev->epfd;
  handle->index = -1;
  return 0;
}

int ev_add(EPollEventLoop *ev, int fd, void *data) {
  if (fcntl(fd, F_SETFD, O_NONBLOCK) == -1) {
    perror("fcntl(O_NONBLOCK)");
    return 1;
  }
  struct epoll_event e = {.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLHUP, .data = {.ptr = data}};
  if (epoll_ctl(ev->epfd, EPOLL_CTL_ADD, fd, &e) == -1) {
    perror("epoll_ctl(add)");
    return 1;
  }
  return 0;
}

static EventKind epoll_events_to_enum(uint32_t events) {
  if (events & (EPOLLRDHUP | EPOLLHUP))
    return EVENT_CLOSE;
  if (events & EPOLLOUT)
    return EVENT_WRITE;
  return EVENT_READ;
}

int ev_next(EPollEventHandle *h, Event *e) {
  if (h->index < 0) {
    int n = epoll_wait(h->epfd, h->events, EV_MAX_EVENTS, -1);
    if (n == -1) {
      perror("epoll_wait");
      return -1;
    }
    h->index = n - 1;
  }
  e->data = h->events[h->index].data.ptr;
  e->kind = epoll_events_to_enum(h->events[h->index].events);
  h->index -= 1;
  return 0;
}

int ev_close(EPollEventLoop *ev) {
  if (close(ev->epfd) == -1) {
    perror("ev_close");
    return 1;
  }
  return 0;
}
