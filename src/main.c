#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/limits.h>
#include "server.h"
#include "http.h"

#define DEFAULT_PORT "8080"

int http_handler(const HttpRequest *req, HttpResponse *res) {
  printf("method=%s\n", http_method_str(req->method));
  printf("uri=%s\n", req->uri);
  for (HttpHeaders *h = req->headers; h; h = h->next)
    printf("%s: %s\n", h->key, h->val);

  const char *body =
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head>\n"
    "<title>Web C Test</title>\n"
    "</head>\n"
    "<body>\n"
    "<h1>Wow, html document from web.c</h1>\n"
    "</body>\n"
    "</html>\n";
  res->body_len = strlen(body);
  res->body = strcpy(malloc(res->body_len + 1), body);
  http_res_add_header(res, "x-webc", strdup("some value here"));
  res->status = 200;
  return 0;
}

int print_usage(const char *program, int error) {
  FILE *out = error ? stderr : stdout;
  fprintf(out, "usage: %s [-h] [-p PORT] [DIR]\n", program);
  if (!error) {
    fprintf(out, "web.c - A small http server written in C\n");
    fprintf(out, "\n");
    fprintf(out, "args:\n");
    fprintf(out, "  DIR      Directory to run web server from, defaults to cwd\n");
    fprintf(out, "  -p PORT  Port to listen on, default " DEFAULT_PORT "\n");
    fprintf(out, "  -h       Show this help text\n");
  }
  return error;
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
      return print_usage(argv[0], 0);
    default:
      return print_usage(argv[0], 1);
    }
  }
  if (optind + 1 < argc) {
    fprintf(stderr, "Unexpected extra positional arguments.\n");
    return print_usage(argv[0], 1);
  }
  char dir[PATH_MAX];
  if (optind + 1 == argc) {
    if (!realpath(argv[optind], dir)) {
      perror("realpath");
      return 1;
    }
  } else if (!getcwd(dir, sizeof(dir))) {
    perror("getcwd");
    return 1;
  }
  (void) dir; // TODO: use the DIR argument

  HttpServer server;
  if (!http_server_init(&server, port))
    return 1;
  printf("Server listening on port %s...\n", port);
  return http_server_run(&server, http_handler);
}
