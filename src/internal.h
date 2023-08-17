#ifndef WEBB_INTERNAL_H
#define WEBB_INTERNAL_H

#include <stddef.h>
#include "webb/webb.h"

#define MAX_HEADERS  64
#define MAX_BODY_LEN (2 * 1024 * 1024)  // 2mb

typedef enum WebbResult {
  RESULT_OK = 0,
  RESULT_INVALID_HTTP,
  RESULT_OOM,
  RESULT_UNEXPECTED,
  RESULT_DISCONNECTED,
  RESULT_NEED_DATA,
} WebbResult;

typedef enum HttpParseStep {
  PARSE_STEP_INIT = 0,
  PARSE_STEP_HEADERS,
  PARSE_STEP_BODY,
  PARSE_STEP_COMPLETE,
} HttpParseStep;

typedef struct HttpParseState {
  HttpParseStep step;
  size_t headers;
  size_t read;
  size_t i;
  char buf[4096];
} HttpParseState;

WebbResult http_parse_step(HttpParseState *state, WebbRequest *req);

WebbResult parse_request(int fd, HttpParseState *state, WebbRequest *req);

void http_state_reset(HttpParseState *state);

void http_req_free(WebbRequest *req);

void http_res_free(WebbResponse *res);

#endif
