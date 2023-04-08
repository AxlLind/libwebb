#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "server.h"
#include "common.h"
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
  int sockfd = open_server_socket(port);
  if (sockfd == -1) {
    return 0;
  }
  *server = (HttpServer) { .sockfd = sockfd };
  return 1;
}

int http_server_run(HttpServer *server, HttpHandler *handler_fn) {
  char msg[65536];
  struct sockaddr_storage addr;
  socklen_t addrsize = sizeof(addr);
  while (1) {
    int connfd = accept(server->sockfd, (struct sockaddr *) &addr, &addrsize);
    if (connfd == -1) {
      perror("accept");
      return 1;
    }

    int size = recv(connfd, msg, sizeof(msg) - 1, 0);
    if (size == -1) {
      perror("recv");
      return 1;
    }
    msg[size] = '\0';
    printf("Got msg of size %d:\n%s\n", size, msg);

    HttpRequest req;
    if (http_parse_req(&req, msg) == RESULT_OK) {
      char* response = handler_fn(&req);
      send(connfd, response, strlen(response), 0);
      free(response);
    } else {
      fprintf(stderr, "failed to parse http request!\n");
      char *response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
      send(connfd, response, strlen(response), 0);
    }
    http_req_free(&req);

    if (close(connfd) == -1) {
      perror("close");
    }
  }
}
