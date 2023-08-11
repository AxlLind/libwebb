#ifndef STR_CONNECTION_H
#define STR_CONNECTION_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "internal.h"
#include "webb/webb.h"

#define TMP_NAME "/tmp/libwebb-XXXXXX"

#define CHECK(ok, msg) \
  do {                 \
    if (!(ok)) {       \
      perror(msg);     \
      return 1;        \
    }                  \
  } while (0)

typedef struct {
  HttpConnection c;
  char path[sizeof(TMP_NAME)];
} StrConnection;

int str_conn_open(StrConnection *conn, const char *s) {
  memset(conn, 0, sizeof(*conn));
  memcpy(conn->path, TMP_NAME, sizeof(TMP_NAME));
  ssize_t len = (ssize_t) strlen(s);
  conn->c.fd = mkstemp(conn->path);
  CHECK(conn->c.fd != -1, "failed to open tmpfile");
  CHECK(write(conn->c.fd, s, len) == len, "failed to write to tmpfile");
  CHECK(lseek(conn->c.fd, 0, SEEK_SET) != -1, "failed to lseek tmpfile");
  return 0;
}

int str_conn_reopen(StrConnection *conn, const char *s) {
  int fd = conn->c.fd;
  memset(&conn->c, 0, sizeof(conn->c));
  conn->c.fd = fd;
  ssize_t len = (ssize_t) strlen(s);
  CHECK(ftruncate(conn->c.fd, 0) != -1, "failed to truncate tmpfile");
  CHECK(pwrite(conn->c.fd, s, len, 0) == len, "failed to write to tmpfile");
  CHECK(lseek(conn->c.fd, 0, SEEK_SET) != -1, "failed to lseek tmpfile");
  return 0;
}

int str_conn_close(StrConnection *conn) {
  CHECK(unlink(conn->path) != -1, "failed to unlink tmpfile");
  CHECK(close(conn->c.fd) != -1, "failed to close tmpfile");
  return 0;
}

#endif
