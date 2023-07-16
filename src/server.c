#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "server.h"
#include "http.h"

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

int http_server_init(HttpServer *server, const char *port) {
  server->sockfd = open_server_socket(port);
  return server->sockfd == -1 ? 0 : 1;
}

static int send_all(int fd, const char *buf, int len) {
  for (int sent = -1; len; buf += sent, len -= sent) {
    sent = send(fd, buf, len, 0);
    if (sent == -1) {
      perror("send");
      return 1;
    }
  }
  return 0;
}

static int send_response(int connfd, const HttpResponse *res) {
  char buf[65536], *bufptr = buf;
  const char *status_str = http_status_str(res->status);
  if (!status_str)
    return 1;
  bufptr += sprintf(bufptr, "HTTP/1.1 %d %s\r\n", res->status, status_str);
  for (HttpHeaders *h = res->headers; h; h = h->next)
    bufptr += sprintf(bufptr, "%s: %s\r\n", h->key, h->val);

  time_t now = time(0);
  struct tm *tm = gmtime(&now);
  bufptr += strftime(bufptr, buf + sizeof(buf) - bufptr, "date: %a, %d %b %Y %H:%M:%S %Z\r\n", tm);
  bufptr += sprintf(bufptr, "server: webc 0.1\r\n");
  bufptr += sprintf(bufptr, "connection: close\r\n");
  if (res->body_len)
    bufptr += sprintf(bufptr, "content-length: %d\r\n", res->body_len);
  bufptr += sprintf(bufptr, "\r\n");
  if (send_all(connfd, buf, bufptr - buf))
    return 1;
  if (res->body && send_all(connfd, res->body, res->body_len))
    return 1;
  return 0;
}

int http_server_run(HttpServer *server, HttpHandler *handler_fn) {
  struct sockaddr_storage addr;
  socklen_t addrsize = sizeof(addr);
  while (1) {
    int connfd = accept(server->sockfd, (struct sockaddr *) &addr, &addrsize);
    if (connfd == -1) {
      perror("accept");
      break;
    }

    HttpConnection conn = {
      .fd = connfd,
      .i = 0,
      .read = 0,
    };
    HttpRequest req;
    HttpResponse res = {0};

    if (http_parse_req(&conn, &req)) {
      fprintf(stderr, "failed to parse http request!\n");
      res.status = 400;
    } else if (handler_fn(&req, &res)) {
      fprintf(stderr, "Request handler failed!");
      break;
    }
    if (send_response(connfd, &res)) {
      fprintf(stderr, "Sending response failed!\n");
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

const char* http_conn_next(HttpConnection *c) {
  while (1) {
    char *end = memmem(c->buf + c->i, c->read - c->i, "\r\n", 2);
    if (end) {
      *end = '\0';
      int start = c->i;
      c->i = end + 2 - c->buf;
      return c->buf + start;
    }
    if (c->read == sizeof(c->buf))
      return NULL;
    memmove(c->buf, c->buf + c->i, c->read - c->i);
    c->read -= c->i;
    c->i = 0;
    int read = recv(c->fd, c->buf + c->read, sizeof(c->buf) - c->read, 0);
    if (read == -1) {
      perror("recv");
      return NULL;
    }
    if (read == 0)
      return NULL;
    c->read += read;
  }
}
