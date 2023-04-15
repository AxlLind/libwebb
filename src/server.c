#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
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
    sockfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if (sockfd == -1) {
      perror("socket");
      continue;
    }
    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
      perror("setsockopt");
      return -1;
    }
    if (bind(sockfd, info->ai_addr, info->ai_addrlen) == -1) {
      close(sockfd);
      perror("bind");
      continue;
    }
    break;
  }
  freeaddrinfo(servinfo);

  if (sockfd == -1) {
    fprintf(stderr, "failed to bind to an ip!\n");
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

static char* malloc_str(const char *s) {
  return strcpy(malloc(strlen(s) + 1), s);
}

static int send_response(int connfd, const HttpResponse *res) {
  char buf[65536], *bufptr = buf;
  char *status_str = http_status_str(res->status);
  if (!status_str)
    return 1;
  bufptr += sprintf(bufptr, "HTTP/1.1 %d %s\r\n", res->status, status_str);
  for (HttpHeaders *h = res->headers; h; h = h->next)
    bufptr += sprintf(bufptr, "%s: %s\r\n", h->key, h->val);
  if (res->body_len)
    bufptr += sprintf(bufptr, "content-length: %d\r\n", res->body_len);
  bufptr += sprintf(bufptr, "\r\n");
  if (send_all(connfd, buf, bufptr - buf))
    return 1;
  if (send_all(connfd, res->body, res->body_len))
    return 1;
  return 0;
}

int http_server_run(HttpServer *server, HttpHandler *handler_fn) {
  char msg[65536];
  struct sockaddr_storage addr;
  socklen_t addrsize = sizeof(addr);
  while (1) {
    int connfd = accept(server->sockfd, (struct sockaddr *) &addr, &addrsize);
    if (connfd == -1) {
      perror("accept");
      break;
    }

    int size = recv(connfd, msg, sizeof(msg) - 1, 0);
    if (size == -1) {
      perror("recv");
      break;
    }
    msg[size] = '\0';
    printf("Got msg of size %d:\n%s\n", size, msg);

    HttpRequest req;
    HttpResponse res;
    memset(&res, 0, sizeof(res));
    http_res_add_header(&res, "server", malloc_str("web.c 0.1"));

    if (http_parse_req(&req, msg)) {
      fprintf(stderr, "failed to parse http request!\n");
      res.status = 400;
    } else if (handler_fn(&req, &res)) {
      fprintf(stderr, "Request handler failed!");
      break;
    }
    if (send_response(connfd, &res))
      break;
    http_req_free(&req);
    http_res_free(&res);

    printf("sent response!\n");

    if (close(connfd) == -1) {
      perror("close");
      break;
    }
  }

  close(server->sockfd);
  memset(server, 0, sizeof(*server));
  return 1;
}
