#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "http.h"
#include "server.h"

#define PATH_MAX     4096
#define DEFAULT_PORT "8080"

const char *mime_type(const char *path) {
  static const char *MIME_TYPES[][2] = {
    {".css", "text/css"},
    {".gif", "image/gif"},
    {".htm", "text/html"},
    {".html", "text/html"},
    {".jpeg", "image/jpg"},
    {".jpg", "image/jpg"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".png", "image/png"},
    {".txt", "text/plain"},
  };
  static const int MIME_LEN = sizeof(MIME_TYPES) / sizeof(MIME_TYPES[0]);
  const char *ext = strrchr(path, '.');
  if (ext && ext != path) {
    for (int i = 0; i < MIME_LEN; i++)
      if (strcasecmp(ext, MIME_TYPES[i][0]) == 0)
        return MIME_TYPES[i][1];
  }
  return "text/raw";
}

int is_file(const char *path) {
  struct stat sb;
  if (stat(path, &sb) == -1)
    return 0;
  return S_ISREG(sb.st_mode);
}

char dir[PATH_MAX];

int http_handler(const HttpRequest *req, HttpResponse *res) {
  printf("Request: %s %s %s\n", http_method_str(req->method), req->uri, req->query ? req->query : "");

  if (req->method != HTTP_GET) {
    res->status = 404;
    return 0;
  }

  char file[PATH_MAX];
  if (snprintf(file, PATH_MAX, "%s%s", dir, req->uri) < 0) {
    res->status = 404;
    return 0;
  }

  if (!is_file(file)) {
    res->status = 404;
    return 0;
  }

  FILE *f = fopen(file, "rb");
  if (!f) {
    perror(req->uri);
    return 1;
  }
  if (fseek(f, 0, SEEK_END) != 0) {
    perror("fseek");
    return 1;
  }
  res->body_len = ftell(f);
  rewind(f);

  res->body = malloc(res->body_len + 1);
  if (!res->body)
    return 1;
  if (fread(res->body, res->body_len, 1, f) != 1) {
    perror("fread");
    return 1;
  }
  if (fclose(f) != 0) {
    perror("fclose");
    return 1;
  }

  http_res_add_header(res, "content-type", strdup(mime_type(file)));
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
    case 'p': port = optarg; break;
    case 'h': return print_usage(argv[0], 0);
    default: return print_usage(argv[0], 1);
    }
  }
  if (optind + 1 < argc) {
    fprintf(stderr, "Unexpected extra positional arguments.\n");
    return print_usage(argv[0], 1);
  }
  if (optind + 1 == argc) {
    if (!realpath(argv[optind], dir)) {
      perror("realpath");
      return 1;
    }
  } else if (!getcwd(dir, sizeof(dir))) {
    perror("getcwd");
    return 1;
  }

  int dirlen = strlen(dir);
  if (dir[dirlen - 1] == '/')
    dir[dirlen - 1] = '\0';

  HttpServer server;
  if (!http_server_init(&server, port))
    return 1;
  printf("Server listening on port %s...\n", port);
  return http_server_run(&server, http_handler);
}
