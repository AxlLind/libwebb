#ifndef HTTP_H
#define HTTP_H

#include "common.h"

typedef enum {
  HTTP_CONNECT,
  HTTP_DELETE,
  HTTP_GET,
  HTTP_HEAD,
  HTTP_OPTIONS,
  HTTP_PATCH,
  HTTP_POST,
  HTTP_PUT,
  HTTP_TRACE,
  HTTP_INVALID,
} HttpMethod;

typedef struct HttpHeaders {
  char *key;
  char *val;
  struct HttpHeaders *next;
} HttpHeaders;

typedef struct {
  HttpMethod method;
  char *uri;
  HttpHeaders *headers;
} HttpRequest;

Result http_parse_req(HttpRequest *req, const char *data);

char* http_method_str(HttpMethod m);

void http_req_free(HttpRequest *req);


#endif // HTTP_H
