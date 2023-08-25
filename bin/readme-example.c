#include <string.h>
#include "webb/webb.h"

int http_handler(const WebbRequest *req, WebbResponse *res) {
  // implement behavior given the HTTP request...
  if (req->method != WEBB_GET)
    return 404;

  res->body = strdup("hello world");
  res->body_len = strlen(res->body);
  webb_set_header(res, "content-type", strdup("text/raw"));
  return 200;
}

int main(void) {
  // do some setup...
  return webb_server_run("8080", http_handler);
}
