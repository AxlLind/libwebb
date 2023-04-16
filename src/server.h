#ifndef SERVER_H
#define SERVER_H

#include "http.h"

typedef int (HttpHandler)(const HttpRequest*, HttpResponse*);

typedef struct {
  int sockfd;
} HttpServer;

int http_server_init(HttpServer *server, const char *port);

int http_server_run(HttpServer *server, HttpHandler *handler);

#endif
