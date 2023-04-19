#ifndef HTTP_H
#define HTTP_H

typedef struct {
  int fd;
  char buf[4096];
  int read;
  int i;
} HttpConnection;

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
  HTTP_INVALID = 0,
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

typedef struct {
  int status;
  HttpHeaders *headers;
  char *body;
  int body_len;
} HttpResponse;

int http_parse_req(HttpRequest *req, HttpConnection *conn);

const char* http_method_str(HttpMethod m);

const char* http_status_str(int status);

void http_req_free(HttpRequest *req);

void http_res_free(HttpResponse *res);

const char* http_get_header(HttpHeaders *h, char *key);

void http_res_add_header(HttpResponse *res, char *key, char *val);

#endif // HTTP_H
