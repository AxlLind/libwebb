#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "internal.h"
#include "webb/webb.h"

static char *uri_decode(const char *s, size_t len) {
  // clang-format off
  static const char HEX_LOOKUP[256] = {
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
      char a = HEX_LOOKUP[(unsigned char) s[++i]];
      char b = HEX_LOOKUP[(unsigned char) s[++i]];
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

static WebbMethod parse_http_method(const char *method, size_t len) {
  if (len == 7 && memcmp(method, "CONNECT", 7) == 0)
    return WEBB_CONNECT;
  if (len == 6 && memcmp(method, "DELETE", 6) == 0)
    return WEBB_DELETE;
  if (len == 3 && memcmp(method, "GET", 3) == 0)
    return WEBB_GET;
  if (len == 4 && memcmp(method, "HEAD", 4) == 0)
    return WEBB_HEAD;
  if (len == 7 && memcmp(method, "OPTIONS", 7) == 0)
    return WEBB_OPTIONS;
  if (len == 5 && memcmp(method, "PATCH", 5) == 0)
    return WEBB_PATCH;
  if (len == 4 && memcmp(method, "POST", 4) == 0)
    return WEBB_POST;
  if (len == 3 && memcmp(method, "PUT", 3) == 0)
    return WEBB_PUT;
  if (len == 5 && memcmp(method, "TRACE", 5) == 0)
    return WEBB_TRACE;
  return WEBB_INVALID;
}

const char *http_next_line(HttpParseState *s) {
  for (size_t i = s->i; i + 1 < s->read; i++) {
    if (s->buf[i] == '\r' && s->buf[i + 1] == '\n') {
      char *line = s->buf + s->i;
      s->i = i + 2;
      s->buf[i] = '\0';
      return line;
    }
  }
  return NULL;
}

WebbResult http_parse_step(HttpParseState *state, WebbRequest *req) {
  while (1) {
    switch (state->step) {
    case PARSE_STEP_INIT: {
      memset(req, 0, sizeof(*req));
      const char *line = http_next_line(state);
      if (!line) {
        if (state->read == sizeof(state->buf))
          return RESULT_INVALID_HTTP;
        return RESULT_NEED_DATA;
      }

      // parse http verb, ends with a space
      char *verb_end = strchr(line, ' ');
      if (!verb_end)
        return RESULT_INVALID_HTTP;
      req->method = parse_http_method(line, verb_end - line);
      if (req->method == WEBB_INVALID)
        return RESULT_INVALID_HTTP;

      // parse uri, ends with a space
      line = verb_end + 1;
      char *qs_end = strchr(line, ' '), *uri_end = qs_end;
      if (!qs_end)
        return RESULT_INVALID_HTTP;
      char *query_start = memchr(line, '?', qs_end - line);
      if (query_start) {
        req->query = strndup(query_start + 1, qs_end - query_start - 1);
        uri_end = query_start;
      }
      req->uri = uri_decode(line, uri_end - line);
      if (!req->uri)
        return RESULT_INVALID_HTTP;

      // parse the required HTTP/1.1 trailer
      if (strcmp(qs_end + 1, "HTTP/1.1") != 0)
        return RESULT_INVALID_HTTP;
      state->step = PARSE_STEP_HEADERS;
      break;
    }
    case PARSE_STEP_HEADERS: {
      const char *line = http_next_line(state);
      if (!line) {
        if (state->read == sizeof(state->buf))
          return RESULT_INVALID_HTTP;
        return RESULT_NEED_DATA;
      }
      // empty line means end of headers
      if (strcmp(line, "") == 0) {
        state->step = PARSE_STEP_BODY;
        break;
      }

      if (state->headers++ == MAX_HEADERS)
        return RESULT_INVALID_HTTP;

      char *header_mid = strstr(line, ": ");
      if (!header_mid)
        return RESULT_INVALID_HTTP;

      WebbHeaders *header = malloc(sizeof(WebbHeaders));
      header->key = strndup(line, header_mid - line);
      header->val = strdup(header_mid + 2);
      header->next = req->headers;
      req->headers = header;
      break;
    }
    case PARSE_STEP_BODY: {
      const char *content_length = webb_get_header(req, "content-length");
      if (content_length) {
        ssize_t length = strtol(content_length, NULL, 10);
        if (length > 0)
          req->body_len = (size_t) length;
      }
      state->step = PARSE_STEP_COMPLETE;
      break;
    }
    case PARSE_STEP_COMPLETE:
      return RESULT_OK;
    }
  }
}

void http_state_reset(HttpParseState *state) {
  state->step = PARSE_STEP_INIT;
  state->headers = 0;
  state->body_read = 0;
}

const char *webb_get_header(const WebbRequest *req, const char *key) {
  for (WebbHeaders *h = req->headers; h; h = h->next) {
    if (strcasecmp(key, h->key) == 0)
      return h->val;
  }
  return NULL;
}

void webb_set_header(WebbResponse *res, char *key, char *val) {
  WebbHeaders *header = malloc(sizeof(WebbHeaders));
  header->key = key;
  header->val = val;
  header->next = res->headers;
  res->headers = header;
}

const char *webb_method_str(WebbMethod m) {
  // clang-format off
  switch (m) {
  case WEBB_CONNECT: return "CONNECT";
  case WEBB_DELETE:  return "DELETE";
  case WEBB_GET:     return "GET";
  case WEBB_HEAD:    return "HEAD";
  case WEBB_OPTIONS: return "OPTIONS";
  case WEBB_PATCH:   return "PATCH";
  case WEBB_POST:    return "POST";
  case WEBB_PUT:     return "PUT";
  case WEBB_TRACE:   return "TRACE";
  default:           return "INVALID";
  }
  // clang-format on
}

const char *webb_status_str(int status) {
  // clang-format off
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
  // clang-format on
}
