#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "http.h"
#include "common.h"

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

Result http_parse_req(HttpRequest *req, const char *data) {
  memset(req, 0, sizeof(*req));

  // parse http verb, ends with a space
  char *verb_end = strchr(data, ' ');
  req->method = parse_http_method(data, verb_end == NULL ? 0 : verb_end - data);
  if (req->method == HTTP_INVALID)
    return RESULT_FAIL;

  // parse uri, ends with a space
  data = verb_end + 1;
  char *uri_end = strchr(data, ' ');
  if (!uri_end)
    return RESULT_FAIL;
  req->uri = copy_string(data, uri_end);

  // parse the required HTTP/1.1 trailer
  data = uri_end + 1;
  char *start_line_end = strstr(data, "\r\n");
  int trailer_len = start_line_end - data;
  if (trailer_len != 8 || memcmp(data, "HTTP/1.1", 8) != 0)
    return RESULT_FAIL;

  // parse each header
  data = start_line_end + 2;
  while (1) {
    if (data[0] == '\0')
      return RESULT_FAIL;
    // empty line means end of headers
    if (memcmp(data, "\r\n", 2) == 0)
      break;

    // headers are on the form: "KEY: VAL\r\n"
    char *header_mid = strstr(data, ": ");
    if (!header_mid)
      return RESULT_FAIL;
    char *header_end = strstr(header_mid + 2, "\r\n");
    if (!header_end)
      return RESULT_FAIL;

    HttpHeaders *header = malloc(sizeof(HttpHeaders));
    header->key = copy_string(data, header_mid),
    header->val = copy_string(header_mid + 2, header_end),
    header->next = req->headers,
    req->headers = header;

    // http headers are case insensitive so convert to lower
    for (char *key = header->key; *key; key++)
      *key = tolower(*key);

    data = header_end + 2;
  }
  return RESULT_OK;
}

void http_req_free(HttpRequest *req) {
  HttpHeaders *header = req->headers;
  while (header) {
    HttpHeaders *next = header->next;
    free(header->key);
    free(header->val);
    free(header);
    header = next;
  }
  free(req->uri);
}
