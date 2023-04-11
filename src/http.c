#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "http.h"

char* http_method_str(HttpMethod m) {
  switch (m) {
  case HTTP_CONNECT: return "CONNECT";
  case HTTP_DELETE:  return "DELETE";
  case HTTP_GET:     return "GET";
  case HTTP_HEAD:    return "HEAD";
  case HTTP_OPTIONS: return "OPTIONS";
  case HTTP_PATCH:   return "PATCH";
  case HTTP_POST:    return "POST";
  case HTTP_PUT:     return "PUT";
  case HTTP_TRACE:   return "TRACE";
  default:           return "INVALID";
  }
}

static HttpMethod parse_http_method(const char *method, const int len) {
  if (len == 7 && memcmp(method, "CONNECT", 7) == 0)
    return HTTP_CONNECT;
  if (len == 6 && memcmp(method, "DELETE", 6) == 0)
    return HTTP_DELETE;
  if (len == 3 && memcmp(method, "GET", 3) == 0)
    return HTTP_GET;
  if (len == 4 && memcmp(method, "HEAD", 4) == 0)
    return HTTP_HEAD;
  if (len == 7 && memcmp(method, "OPTIONS", 7) == 0)
    return HTTP_OPTIONS;
  if (len == 5 && memcmp(method, "PATCH", 5) == 0)
    return HTTP_PATCH;
  if (len == 4 && memcmp(method, "POST", 4) == 0)
    return HTTP_POST;
  if (len == 3 && memcmp(method, "PUT", 3) == 0)
    return HTTP_PUT;
  if (len == 5 && memcmp(method, "TRACE", 5) == 0)
    return HTTP_TRACE;
  return HTTP_INVALID;
}

static char* copy_string(const char *src, const char *end) {
  int len = end - src;
  char *dst = calloc(len + 1, 1);
  return memcpy(dst, src, len);
}

int http_parse_req(HttpRequest *req, const char *data) {
  memset(req, 0, sizeof(*req));

  // parse http verb, ends with a space
  char *verb_end = strchr(data, ' ');
  req->method = parse_http_method(data, verb_end == NULL ? 0 : verb_end - data);
  if (req->method == HTTP_INVALID)
    return 1;

  // parse uri, ends with a space
  data = verb_end + 1;
  char *uri_end = strchr(data, ' ');
  if (!uri_end)
    return 1;
  req->uri = copy_string(data, uri_end);

  // parse the required HTTP/1.1 trailer
  data = uri_end + 1;
  char *start_line_end = strstr(data, "\r\n");
  int trailer_len = start_line_end - data;
  if (trailer_len != 8 || memcmp(data, "HTTP/1.1", 8) != 0)
    return 1;

  // parse each header
  data = start_line_end + 2;
  while (1) {
    if (data[0] == '\0')
      return 1;
    // empty line means end of headers
    if (memcmp(data, "\r\n", 2) == 0)
      break;

    // headers are on the form: "KEY: VAL\r\n"
    char *header_mid = strstr(data, ": ");
    if (!header_mid)
      return 1;
    char *header_end = strstr(header_mid + 2, "\r\n");
    if (!header_end)
      return 1;

    HttpHeaders *header = malloc(sizeof(HttpHeaders));
    header->key = copy_string(data, header_mid);
    header->val = copy_string(header_mid + 2, header_end);
    header->next = req->headers;
    req->headers = header;

    // http headers are case insensitive so convert to lower
    for (char *key = header->key; *key; key++)
      *key = tolower(*key);

    data = header_end + 2;
  }
  return 0;
}

static void free_headers(HttpHeaders *header, int allocated_keys) {
  while (header) {
    HttpHeaders *next = header->next;
    if (allocated_keys)
      free(header->key);
    free(header->val);
    free(header);
    header = next;
  }
}

void http_req_free(HttpRequest *req) {
  free_headers(req->headers, 1);
  free(req->uri);
}

void http_res_free(HttpResponse *res) {
  free_headers(res->headers, 0);
  free(res->body);
}

void http_res_add_header(HttpResponse *res, char *key, char *val) {
  HttpHeaders *header = malloc(sizeof(HttpHeaders));
  header->key = key;
  header->val = val;
  header->next = res->headers;
  res->headers = header;
}
