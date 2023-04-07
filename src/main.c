#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "common.h"
#include "http.h"

#define DEFAULT_PORT "8080"
#define BACKLOG 10

int open_socket(const char* const port) {
  struct addrinfo *info = NULL;
  struct addrinfo hints = {
    .ai_family = AF_UNSPEC,
    .ai_flags = AI_PASSIVE,
    .ai_socktype = SOCK_STREAM,
  };

  if (getaddrinfo(NULL, port, &hints, &info) != 0) {
    perror("getaddrinfo");
    return -1;
  }

  int sockfd = -1;
  for (; info; info = info->ai_next) {
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
  freeaddrinfo(info);

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

int main(void) {
  int sockfd = open_socket(DEFAULT_PORT);
  if (sockfd == -1) {
    fprintf(stderr, "Failed to open tcp socket!\n");
    return 1;
  }

  printf("Server listening on port %s...\n", DEFAULT_PORT);
  while (1) {
    struct sockaddr_storage addr;
    socklen_t addrsize = sizeof(addr);
    int connfd = accept(sockfd, (struct sockaddr *) &addr, &addrsize);
    printf("Got connection!\n");

    char msg[65536];
    int size = recv(connfd, msg, sizeof(msg) - 1, 0);
    if (size == -1) {
      perror("recv");
      return 1;
    }
    msg[size] = '\0';
    printf("Got msg of size %d:\n%s\n", size, msg);

    HttpRequest req;
    if (http_parse_req(&req, msg) == RESULT_OK) {
      printf("method=%s\n", http_method_str(req.method));
      printf("uri=%s\n", req.uri);
      for (HttpHeaders *h = req.headers; h; h = h->next)
        printf("%s: %s\n", h->key, h->val);
    } else {
      printf("Failed to parse http request!\n");
    }
    http_req_free(&req);

    char *response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
    send(connfd, response, strlen(response), 0);

    if (close(connfd) == -1) {
      perror("close");
    }
  }
}
