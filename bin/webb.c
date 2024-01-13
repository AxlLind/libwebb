#include <dirent.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "webb/webb.h"

#define PATH_MAX     4096
#define DEFAULT_PORT "8080"

const char *mime_type(const char *path) {
  static const char *const MIME_TYPES[][2] = {
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
    for (int i = 0; i < MIME_LEN; i++) {
      if (strcasecmp(ext, MIME_TYPES[i][0]) == 0)
        return MIME_TYPES[i][1];
    }
  }
  return "text/raw";
}

char WORK_DIR[PATH_MAX];

int handle_dir(WebbResponse *res, const char *path, const char *uri) {
  DIR *dp = opendir(path);
  if (!dp) {
    perror("could not open dir");
    return -1;
  }

  char *html = malloc(65536), *s = html;
  s += sprintf(s, "<!DOCTYPE html>\n<html>\n<body>\n<ul>\n");

  for (struct dirent *entry; (entry = readdir(dp));) {
    if (strcmp(entry->d_name, ".") == 0)
      continue;
    if (strcmp(entry->d_name, "..") == 0)
      continue;
    s += sprintf(
      s,
      "<li><a href=\"%s%s%s\">%s</a></li>\n",
      uri,
      uri[strlen(uri) - 1] == '/' ? "" : "/",
      entry->d_name,
      entry->d_name);
  }
  closedir(dp);

  s += sprintf(s, "</ul>\n</body>\n</html>\n");
  webb_set_body(res, html, s - html);
  webb_set_header(res, "content-type", strdup("text/html"));
  return 200;
}

int handle_request(const WebbRequest *req, WebbResponse *res) {
  if (req->method != WEBB_GET)
    return 404;

  char input_path[PATH_MAX], file[PATH_MAX];
  if (snprintf(input_path, PATH_MAX, "%s%s", WORK_DIR, req->uri) < 0)
    return 404;
  if (realpath(input_path, file) == NULL)
    return 404;
  // check path traversal attack
  if (strstr(file, WORK_DIR) != file)
    return 404;

  struct stat sb;
  if (stat(file, &sb) == -1)
    return 404;
  if (S_ISDIR(sb.st_mode))
    return handle_dir(res, file, req->uri);
  if (!S_ISREG(sb.st_mode))
    return 404;

  int fd = open(file, O_RDONLY);
  if (fd == -1) {
    perror("open");
    return 500;
  }
  webb_set_body_fd(res, fd, sb.st_size);
  webb_set_header(res, "content-type", strdup(mime_type(file)));
  return 200;
}

int http_handler(const WebbRequest *req, WebbResponse *res) {
  int status = handle_request(req, res);
  printf(
    "%s %s%s%s %d\n",
    webb_method_str(req->method),
    req->uri,
    req->query ? "?" : "",
    req->query ? req->query : "",
    status);
  return status;
}

int print_usage(const char *program, int error) {
  printf("usage: %s [-h] [-p PORT] [DIR]\n", program);
  if (!error) {
    printf("webb - A small http server written in C using libwebb\n");
    printf("\n");
    printf("args:\n");
    printf("  DIR      Directory to run web server from, defaults to cwd\n");
    printf("  -p PORT  Port to listen on, default " DEFAULT_PORT "\n");
    printf("  -h       Show this help text\n");
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
    printf("Unexpected extra positional arguments.\n");
    return print_usage(argv[0], 1);
  }
  if (optind + 1 == argc) {
    if (!realpath(argv[optind], WORK_DIR)) {
      perror("realpath");
      return 1;
    }
  } else if (!getcwd(WORK_DIR, sizeof(WORK_DIR))) {
    perror("getcwd");
    return 1;
  }

  size_t dirlen = strlen(WORK_DIR);
  if (WORK_DIR[dirlen - 1] == '/')
    WORK_DIR[dirlen - 1] = '\0';

  printf("Server listening on port %s...\n", port);
  return webb_server_run(port, http_handler);
}
