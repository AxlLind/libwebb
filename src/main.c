#include <stdio.h>
#include "common.h"
#include "http.h"

static const char* TEST_REQUEST =
  "GET /test.txt HTTP/1.1\r\n"
  "Host: 127.0.0.1\r\n"
  "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/110.0.0.0 Safari/537.36\r\n"
  "X-custom-header: Custom header\nwith a newline\r\n"
  "\r\n";

int main(void) {
  HttpRequest req;
  Result res = http_parse_req(&req, TEST_REQUEST);
  if (res != RESULT_OK) {
    printf("Parsing failed!\n");
    return 1;
  }
  printf("method=%s\n", http_method_str(req.method));
  printf("uri=%s\n", req.uri);
  printf("headers:\n");
  for (HttpHeaders *h = req.headers; h; h = h->next) {
    printf("%s: %s\n", h->header, h->value);
  }
  http_req_free(&req);
}
