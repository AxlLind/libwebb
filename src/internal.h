#ifndef WEBB_INTERNAL_H
#define WEBB_INTERNAL_H

#include <stddef.h>
#include "webb/webb.h"

typedef struct {
  int fd;
  size_t read;
  size_t i;
  char buf[4096];
} HttpConnection;

int http_parse_req(HttpConnection *conn, WebbRequest *req);

void http_req_free(WebbRequest *req);

void http_res_free(WebbResponse *res);

const char *http_conn_next(HttpConnection *conn);

#endif