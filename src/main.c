#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/limits.h>
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

int print_usage(const char *program, int with_description) {
  FILE *out = with_description ? stdout : stderr;
  fprintf(out, "usage: %s [-h] [-p PORT] DIR\n", program);
  if (with_description) {
    fprintf(out, "web.c - A small http server written in C\n");
    fprintf(out, "\n");
    fprintf(out, "args:\n");
    fprintf(out, "  DIR      Directory to run web server from\n");
    fprintf(out, "  -p PORT  Port to listen on, default " DEFAULT_PORT "\n");
    fprintf(out, "  -h       Show this help text\n");
  }
  return with_description ? 0 : 1;
}

int main(int argc, char *argv[]) {
  int opt;
  char *port = DEFAULT_PORT;
  while ((opt = getopt(argc, argv, "p:h")) != -1) {
    switch (opt) {
    case 'p':
      port = optarg;
      break;
    case 'h':
      return print_usage(argv[0], 1);
    default:
      return print_usage(argv[0], 0);
    }
  }
  if (optind + 1 < argc) {
    fprintf(stderr, "Unexpected extra positional arguments.\n");
    return print_usage(argv[0], 0);
  }
  if (optind + 1 != argc) {
    fprintf(stderr, "Missing positional argument DIR\n");
    return print_usage(argv[0], 0);
  }

  char dir[PATH_MAX];
  if (!realpath(argv[optind], dir)) {
    perror("realpath");
    return 1;
  }
  (void) dir; // TODO: use the DIR argument

  HttpServer server;
  if (!http_server_init(&server, port))
    return 1;
  printf("Server listening on port %s...\n", port);
  return http_server_run(&server, http_handler);
}
