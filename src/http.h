#ifndef HTTP_H
#define HTTP_H

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

typedef struct {
  int status;
  HttpHeaders *headers;
  char *body;
  int body_len;
} HttpResponse;

int http_parse_req(HttpRequest *req, const char *data);

char* http_method_str(HttpMethod m);

char* http_status_str(int status);

void http_req_free(HttpRequest *req);

void http_res_free(HttpResponse *res);

void http_res_add_header(HttpResponse *res, char *key, char *val);

#endif // HTTP_H
