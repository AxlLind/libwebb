#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "server.h"
#include "http.h"

#define DEFAULT_PORT "8080"

char* http_handler(HttpRequest *req) {
  printf("method=%s\n", http_method_str(req->method));
  printf("uri=%s\n", req->uri);
  for (HttpHeaders *h = req->headers; h; h = h->next)
    printf("%s: %s\n", h->key, h->val);

  char *res = malloc(32);
  strcpy(res, "HTTP/1.1 404 Not Found\r\n\r\n");
  return res;
}

int main(void) {
  HttpServer server;
  if (!http_server_init(&server, DEFAULT_PORT))
    return 1;
  printf("Server listening on port %s...\n", DEFAULT_PORT);
  return http_server_run(&server, http_handler);
}
