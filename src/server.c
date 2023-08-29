#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "ev_epoll.h"
#include "internal.h"
#include "webb/webb.h"

typedef struct ThreadPayload {
  pthread_t tid;
  EPollEventLoop ev;
  WebbHandler *handler_fn;
} ThreadPayload;

typedef struct Connection {
  int fd;
  WebbRequest req;
  HttpParseState state;
} Connection;

static int open_server_socket(const char *port) {
  struct addrinfo *servinfo = NULL;
  struct addrinfo hints = {
    .ai_family = AF_UNSPEC,
    .ai_flags = AI_PASSIVE,
    .ai_socktype = SOCK_STREAM,
  };

  if (getaddrinfo(NULL, port, &hints, &servinfo) != 0) {
    perror("getaddrinfo");
    return -1;
  }

  int sockfd = -1;
  for (struct addrinfo *info = servinfo; info; info = info->ai_next) {
    int fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if (fd == -1)
      continue;
    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
      perror("setsockopt");
      return -1;
    }
    if (bind(fd, info->ai_addr, info->ai_addrlen) == -1) {
      close(fd);
      continue;
    }
    sockfd = fd;
    break;
  }
  freeaddrinfo(servinfo);

  if (sockfd == -1) {
    perror("failed to bind to an ip");
    return -1;
  }
  if (listen(sockfd, 16) == -1) {
    perror("listen");
    return -1;
  }
  return sockfd;
}

static int send_all(int fd, const char *buf, size_t len) {
  for (ssize_t sent = -1; len; buf += sent, len -= sent) {
    sent = send(fd, buf, len, 0);
    if (sent == -1) {
      if (errno == EWOULDBLOCK) {
        LOG("writing would block!");
        continue;
      }
      perror("send");
      return 1;
    }
  }
  return 0;
}

static int send_response(int connfd, const WebbResponse *res) {
  char buf[65536], *bufptr = buf;
  const char *status_str = webb_status_str(res->status);
  if (!status_str)
    return 1;
  bufptr += sprintf(bufptr, "HTTP/1.1 %d %s\r\n", res->status, status_str);
  for (WebbHeaders *h = res->headers; h; h = h->next)
    bufptr += sprintf(bufptr, "%s: %s\r\n", h->key, h->val);

  time_t now = time(0);
  struct tm *tm = gmtime(&now);
  bufptr += strftime(bufptr, buf + sizeof(buf) - bufptr, "date: %a, %d %b %Y %H:%M:%S %Z\r\n", tm);
  bufptr += sprintf(bufptr, "server: libwebb 0.1\r\n");
  bufptr += sprintf(bufptr, "connection: keep-alive\r\n");
  bufptr += sprintf(bufptr, "content-length: %zu\r\n", res->body_len);
  bufptr += sprintf(bufptr, "\r\n");
  if (send_all(connfd, buf, bufptr - buf))
    return 1;
  if (res->body && send_all(connfd, res->body, res->body_len))
    return 1;
  return 0;
}

WebbResult parse_request(int fd, HttpParseState *s, WebbRequest *req) {
  // this function has to be reentrant at every EWOULDBLOCK point
  while (s->step != PARSE_STEP_COMPLETE) {
    WebbResult res = http_parse_step(s, req);
    switch (res) {
    case RESULT_OK:
      break;
    case RESULT_NEED_DATA:
      memmove(s->buf, s->buf + s->i, s->read - s->i);
      s->read -= s->i;
      s->i = 0;
      ssize_t nread = read(fd, s->buf + s->read, sizeof(s->buf) - s->read);
      if (nread == -1) {
        if (errno == EWOULDBLOCK)
          return RESULT_NEED_DATA;
        perror("read");
        return RESULT_UNEXPECTED;
      }
      if (nread == 0)
        return RESULT_DISCONNECTED;
      s->read += nread;
      break;
    default:
      return res;
    }
  }

  if (req->body_len == 0)
    return RESULT_OK;

  if (req->body_len > (size_t) MAX_BODY_LEN)
    return RESULT_INVALID_HTTP;
  if (!req->body) {
    req->body = malloc(req->body_len + 1);
    if (!req->body)
      return RESULT_OOM;
    size_t i = s->read - s->i;
    memcpy(req->body, s->buf + s->i, i > req->body_len ? req->body_len : i);
    s->i = s->read;
    s->body_read = i;
  }
  for (ssize_t nread = -1; s->body_read < req->body_len; s->body_read += nread) {
    nread = read(fd, req->body + s->body_read, req->body_len - s->body_read);
    if (nread == -1) {
      if (errno == EWOULDBLOCK)
        return RESULT_NEED_DATA;
      perror("read");
      return RESULT_UNEXPECTED;
    }
    if (nread == 0)
      return RESULT_DISCONNECTED;
  }
  return RESULT_OK;
}

static void *worker_thread(void *arg) {
  ThreadPayload *payload = arg;
  Event event;
  while (ev_next(&payload->ev, &event) == 0) {
    Connection *conn = event.data;
    switch (event.kind) {
    case EVENT_READ:
      switch (parse_request(conn->fd, &conn->state, &conn->req)) {
      case RESULT_OK: {
        WebbResponse res = {0};
        res.status = payload->handler_fn(&conn->req, &res);
        if (res.status < 0) {
          LOG("handler function failed");
          res.status = 500;
        }
        if (send_response(conn->fd, &res) != 0)
          LOG("failed to send data");
        http_res_free(&res);
        http_req_free(&conn->req);
        http_state_reset(&conn->state);
        break;
      }
      case RESULT_NEED_DATA:
        continue;
      case RESULT_DISCONNECTED:
        if (close(conn->fd) == -1)
          perror("close");
        free(conn);
        break;
      default:
        LOG("unexpected error");
        break;
      }
      break;
    case EVENT_WRITE:
      LOG("TODO: should not get here");
      exit(1);
    case EVENT_CLOSE:
      if (close(conn->fd) == -1)
        perror("close");
      free(conn);
    }
  }
  LOG("fatal error in worker thread!");
  return NULL;
}

int webb_server_run(const char *port, WebbHandler *handler_fn) {
  int sockfd = open_server_socket(port);
  if (sockfd == -1)
    return 1;

  ThreadPayload payloads[8];
  for (int i = 0; i < 8; i++) {
    payloads[i].handler_fn = handler_fn;
    if (ev_create(&payloads[i].ev) != 0)
      goto err;
    if (pthread_create(&payloads[i].tid, NULL, worker_thread, &payloads[i]) != 0) {
      perror("pthread_create");
      goto err;
    }
  }

  struct sockaddr_storage addr;
  socklen_t addrsize = sizeof(addr);
  for (size_t tid = 0; 1; tid = (tid + 1) % 8) {
    Connection *conn = calloc(1, sizeof(Connection));
    if (!conn) {
      LOG("failed to allocate new connection");
      goto err;
    }
    conn->fd = accept(sockfd, (struct sockaddr *) &addr, &addrsize);
    if (conn->fd == -1) {
      perror("accept");
      free(conn);
      goto err;
    }
    if (ev_add(&payloads[tid].ev, conn->fd, conn) != 0) {
      close(conn->fd);
      free(conn);
      goto err;
    }
  }

err:
  (void) close(sockfd);
  return 1;
}
