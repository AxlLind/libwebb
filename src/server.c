#include <netdb.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "internal.h"
#include "webb/webb.h"

#define BACKLOG 10

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
  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    return -1;
  }
  return sockfd;
}

int webb_server_init(WebbServer *server, const char *port) {
  server->sockfd = open_server_socket(port);
  return server->sockfd == -1 ? 1 : 0;
}

static int send_all(int fd, const char *buf, size_t len) {
  for (ssize_t sent = -1; len; buf += sent, len -= sent) {
    sent = send(fd, buf, len, 0);
    if (sent == -1) {
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
  bufptr += sprintf(bufptr, "connection: close\r\n");
  if (res->body_len)
    bufptr += sprintf(bufptr, "content-length: %zu\r\n", res->body_len);
  bufptr += sprintf(bufptr, "\r\n");
  if (send_all(connfd, buf, bufptr - buf))
    return 1;
  if (res->body && send_all(connfd, res->body, res->body_len))
    return 1;
  return 0;
}

int webb_server_run(WebbServer *server, WebbHandler *handler_fn) {
  struct sockaddr_storage addr;
  socklen_t addrsize = sizeof(addr);
  while (1) {
    int connfd = accept(server->sockfd, (struct sockaddr *) &addr, &addrsize);
    if (connfd == -1) {
      perror("accept");
      break;
    }

    HttpConnection conn = {.fd = connfd};
    WebbRequest req;
    WebbResponse res = {0};

    if (http_parse_req(&conn, &req)) {
      (void) fprintf(stderr, "failed to parse http request!\n");
      res.status = 400;
    } else {
      res.status = handler_fn(&req, &res);
      if (res.status < 0) {
        (void) fprintf(stderr, "Request handler failed!\n");
        res.status = 500;
      }
    }
    if (send_response(connfd, &res)) {
      (void) fprintf(stderr, "Sending response failed!\n");
      break;
    }
    http_req_free(&req);
    http_res_free(&res);

    if (close(connfd) == -1) {
      perror("close");
      break;
    }
  }

  close(server->sockfd);
  memset(server, 0, sizeof(*server));
  return 1;
}

const char *http_conn_next(HttpConnection *c) {
  while (1) {
    for (size_t i = c->i; i + 1 < c->read; i++) {
      if (memcmp(c->buf + i, "\r\n", 2) == 0) {
        char *res = c->buf + c->i;
        c->i = i + 2;
        c->buf[i] = '\0';
        return res;
      }
    }
    if (c->read == sizeof(c->buf))
      return NULL;

    memmove(c->buf, c->buf + c->i, c->read - c->i);
    c->read -= c->i;
    c->i = 0;

    ssize_t nread = read(c->fd, c->buf + c->read, sizeof(c->buf) - c->read);
    if (nread == -1) {
      perror("recv");
      return NULL;
    }
    if (nread == 0)
      return NULL;
    c->read += nread;
  }
}

int http_conn_read_buf(HttpConnection *c, char *buf, size_t len) {
  size_t i = c->read - c->i;
  memcpy(buf, c->buf + c->i, i > len ? len : i);
  c->i = c->read;

  for (ssize_t nread = -1; i < len; i += nread) {
    nread = read(c->fd, buf + i, len - i);
    if (nread == -1) {
      perror("read");
      return 1;
    }
    if (nread == 0)
      return 1;
  }
  return 0;
}
