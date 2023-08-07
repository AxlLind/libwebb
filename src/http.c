#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "http.h"
#include "server.h"

const char *http_method_str(HttpMethod m) {
  switch (m) {
  case HTTP_CONNECT: return "CONNECT";
  case HTTP_DELETE: return "DELETE";
  case HTTP_GET: return "GET";
  case HTTP_HEAD: return "HEAD";
  case HTTP_OPTIONS: return "OPTIONS";
  case HTTP_PATCH: return "PATCH";
  case HTTP_POST: return "POST";
  case HTTP_PUT: return "PUT";
  case HTTP_TRACE: return "TRACE";
  default: return "INVALID";
  }
}

const char *http_status_str(int status) {
  switch (status) {
  case 100: return "Continue";
  case 101: return "Switching protocols";
  case 102: return "Processing";
  case 103: return "Early Hints";
  case 200: return "OK";
  case 201: return "Created";
  case 202: return "Accepted";
  case 203: return "Non-Authoritative Information";
  case 204: return "No Content";
  case 205: return "Reset Content";
  case 206: return "Partial Content";
  case 207: return "Multi-Status";
  case 208: return "Already Reported";
  case 226: return "IM Used";
  case 300: return "Multiple Choices";
  case 301: return "Moved Permanently";
  case 302: return "Found";
  case 303: return "See Other";
  case 304: return "Not Modified";
  case 305: return "Use Proxy";
  case 306: return "Switch Proxy";
  case 307: return "Temporary Redirect";
  case 308: return "Permanent Redirect";
  case 400: return "Bad Request";
  case 401: return "Unauthorized";
  case 402: return "Payment Required";
  case 403: return "Forbidden";
  case 404: return "Not Found";
  case 405: return "Method Not Allowed";
  case 406: return "Not Acceptable";
  case 407: return "Proxy Authentication Required";
  case 408: return "Request Timeout";
  case 409: return "Conflict";
  case 410: return "Gone";
  case 411: return "Length Required";
  case 412: return "Precondition Failed";
  case 413: return "Payload Too Large";
  case 414: return "URI Too Long";
  case 415: return "Unsupported Media Type";
  case 416: return "Range Not Satisfiable";
  case 417: return "Expectation Failed";
  case 418: return "I'm a Teapot";
  case 421: return "Misdirected Request";
  case 422: return "Unprocessable Entity";
  case 423: return "Locked";
  case 424: return "Failed Dependency";
  case 425: return "Too Early";
  case 426: return "Upgrade Required";
  case 428: return "Precondition Required";
  case 429: return "Too Many Requests";
  case 431: return "Request Header Fields Too Large";
  case 451: return "Unavailable For Legal Reasons";
  case 500: return "Internal Server Error";
  case 501: return "Not Implemented";
  case 502: return "Bad Gateway";
  case 503: return "Service Unavailable";
  case 504: return "Gateway Timeout";
  case 505: return "HTTP Version Not Supported";
  case 506: return "Variant Also Negotiates";
  case 507: return "Insufficient Storage";
  case 508: return "Loop Detected";
  case 510: return "Not Extended";
  case 511: return "Network Authentication Required";
  default: return NULL;
  }
}

static char *uri_decode(const char *s, size_t len) {
  // clang-format off
  static const char tbl[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
     0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1,
    -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  };
  // clang-format on
  char *res = malloc(len + 1), *dst = res;
  for (size_t i = 0; i < len; i++) {
    char c = s[i];
    if (c == '+') {
      c = ' ';
    } else if (c == '%') {
      if (i + 2 >= len)
        goto err;
      char a = tbl[(unsigned char) s[++i]];
      char b = tbl[(unsigned char) s[++i]];
      if (a < 0 || b < 0)
        goto err;
      c = (char) ((a << 4) | b);
    }
    *dst++ = c;
  }
  *dst = '\0';
  return res;

err:
  free(res);
  return NULL;
}

static HttpMethod parse_http_method(const char *method, size_t len) {
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

int http_parse_req(HttpConnection *conn, HttpRequest *req) {
  memset(req, 0, sizeof(*req));

  // parse http verb, ends with a space
  const char *line = http_conn_next(conn);
  if (!line)
    return 1;
  char *verb_end = strchr(line, ' ');
  req->method = parse_http_method(line, verb_end ? verb_end - line : 0);
  if (req->method == HTTP_INVALID)
    return 1;

  // parse uri, ends with a space
  line = verb_end + 1;
  char *qs_end = strchr(line, ' '), *uri_end = qs_end;
  if (!qs_end)
    return 1;
  char *query_start = memchr(line, '?', qs_end - line);
  if (query_start) {
    req->query = strndup(query_start + 1, qs_end - query_start - 1);
    uri_end = query_start;
  }
  req->uri = uri_decode(line, uri_end - line);
  if (!req->uri)
    return 1;

  // parse the required HTTP/1.1 trailer
  if (strcmp(qs_end + 1, "HTTP/1.1") != 0)
    return 1;

  // parse each header
  while (1) {
    line = http_conn_next(conn);
    if (!line)
      return 1;

    // empty line means end of headers
    if (strcmp(line, "") == 0)
      break;

    char *header_mid = strstr(line, ": ");
    if (!header_mid)
      return 1;

    HttpHeaders *header = malloc(sizeof(HttpHeaders));
    header->key = strndup(line, header_mid - line);
    header->val = strdup(header_mid + 2);
    header->next = req->headers;
    req->headers = header;
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
  free(req->query);
}

void http_res_free(HttpResponse *res) {
  free_headers(res->headers, 0);
  free(res->body);
}

const char *http_get_header(const HttpHeaders *h, const char *key) {
  for (; h; h = h->next) {
    if (strcasecmp(key, h->key) == 0)
      return h->val;
  }
  return NULL;
}

void http_res_add_header(HttpResponse *res, char *key, char *val) {
  HttpHeaders *header = malloc(sizeof(HttpHeaders));
  header->key = key;
  header->val = val;
  header->next = res->headers;
  res->headers = header;
}
