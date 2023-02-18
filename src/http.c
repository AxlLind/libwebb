#include <stdlib.h>
#include <string.h>
#include "http.h"
#include "common.h"

char* http_method_str(HttpMethod m) {
  switch (m) {
  case HTTP_CONNECT:
    return "CONNECT";
  case HTTP_DELETE:
    return "DELETE";
  case HTTP_GET:
    return "GET";
  case HTTP_HEAD:
    return "HEAD";
  case HTTP_OPTIONS:
    return "OPTIONS";
  case HTTP_PATCH:
    return "PATCH";
  case HTTP_POST:
    return "POST";
  case HTTP_PUT:
    return "PUT";
  case HTTP_TRACE:
    return "TRACE";
  default:
    return "INVALID";
  }
}

static HttpMethod parse_http_method(const char *method) {
  if (memcmp(method, "CONNECT", 7) == 0)
    return HTTP_CONNECT;
  if (memcmp(method, "DELETE", 6) == 0)
    return HTTP_DELETE;
  if (memcmp(method, "GET", 3) == 0)
    return HTTP_GET;
  if (memcmp(method, "HEAD", 4) == 0)
    return HTTP_HEAD;
  if (memcmp(method, "OPTIONS", 7) == 0)
    return HTTP_OPTIONS;
  if (memcmp(method, "PATCH", 5) == 0)
    return HTTP_PATCH;
  if (memcmp(method, "POST", 4) == 0)
    return HTTP_POST;
  if (memcmp(method, "PUT", 3) == 0)
    return HTTP_PUT;
  if (memcmp(method, "TRACE", 5) == 0)
    return HTTP_TRACE;
  return HTTP_INVALID;
}

Result http_parse_req(HttpRequest *req, const char *data) {
  memset(req, 0, sizeof(*req));
  req->method = parse_http_method(data);
  if (req->method == HTTP_INVALID)
    return RESULT_FAIL;
  // TODO: Just parse the rest
  return RESULT_OK;
}

void http_req_free(HttpRequest *req) {
  HttpHeaders *header = req->headers;
  while (header) {
    HttpHeaders *next = header->next;
    free(header->header);
    free(header->value);
    free(header);
    header = next;
  }
  free(req->uri);
}
