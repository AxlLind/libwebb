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
#include "internal.h"
#include "webb/webb.h"

#define BACKLOG 10

typedef struct ThreadPayload {
  pthread_t tid;
  int fd;
  WebbHandler *handler_fn;
} ThreadPayload;

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

WebbResult parse_request(int fd, HttpParseState *state, WebbRequest *req) {
  while (state->step != PARSE_STEP_COMPLETE) {
    WebbResult res = http_parse_step(state, req);
    switch (res) {
    case RESULT_OK: break;
    case RESULT_NEED_DATA:
      memmove(state->buf, state->buf + state->i, state->read - state->i);
      state->read -= state->i;
      state->i = 0;
      ssize_t nread = read(fd, state->buf + state->read, sizeof(state->buf) - state->read);
      if (nread == -1) {
        perror("read");
        return RESULT_UNEXPECTED;
      }
      if (nread == 0)
        return RESULT_DISCONNECTED;
      state->read += nread;
      break;
    default: return res;
    }
  }

  if (req->body_len > (size_t) MAX_BODY_LEN)
    return RESULT_INVALID_HTTP;

  // read the body if we have one
  if (req->body_len > 0) {
    req->body = malloc(req->body_len + 1);
    if (!req->body)
      return RESULT_OOM;
    size_t i = state->read - state->i;
    memcpy(req->body, state->buf + state->i, i > req->body_len ? req->body_len : i);
    state->i = state->read;
    for (ssize_t nread = -1; i < req->body_len; i += nread) {
      nread = read(fd, req->body + i, req->body_len - i);
      if (nread == -1) {
        perror("read");
        return RESULT_UNEXPECTED;
      }
      if (nread == 0)
        return RESULT_DISCONNECTED;
    }
  }
  return RESULT_OK;
}

static void *handle_request(void *arg) {
  ThreadPayload *payload = arg;
  HttpParseState state = {0};
  WebbRequest req = {0};
  WebbResponse res = {0};
  switch (parse_request(payload->fd, &state, &req)) {
  case RESULT_OK:
    res.status = payload->handler_fn(&req, &res);
    if (res.status < 0) {
      (void) fprintf(stderr, "Request handler failed!\n");
      res.status = 500;
    }
    break;
  case RESULT_INVALID_HTTP:
    (void) fprintf(stderr, "failed to parse http request!\n");
    res.status = 400;
    break;
  case RESULT_DISCONNECTED: goto done;
  default:
    (void) fprintf(stderr, "unexpected error when parsing request!\n");
    res.status = 500;
    break;
  }
  if (send_response(payload->fd, &res))
    (void) fprintf(stderr, "sending response failed!\n");
  http_req_free(&req);
  http_res_free(&res);

done:
  if (close(payload->fd) == -1)
    perror("close");
  free(payload);
  return NULL;
}

int webb_server_run(const char *port, WebbHandler *handler_fn) {
  int sockfd = open_server_socket(port);
  if (sockfd == -1)
    return 1;
  struct sockaddr_storage addr;
  socklen_t addrsize = sizeof(addr);
  while (1) {
    int fd = accept(sockfd, (struct sockaddr *) &addr, &addrsize);
    if (fd == -1) {
      perror("accept");
      break;
    }

    ThreadPayload *payload = malloc(sizeof(ThreadPayload));
    payload->fd = fd;
    payload->handler_fn = handler_fn;
    if (pthread_create(&payload->tid, NULL, handle_request, payload) != 0) {
      perror("pthread_create");
      free(payload);
      break;
    }
  }
  close(sockfd);
  return 1;
}
