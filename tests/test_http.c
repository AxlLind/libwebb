#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "http.h"
#include "server.h"
#include "test.h"

#define TMP_NAME "/tmp/webc-XXXXXX"

typedef struct {
  HttpConnection c;
  char path[sizeof(TMP_NAME)];
} StrConnection;

int str_conn_open(StrConnection *conn, char *s) {
  memset(conn, 0, sizeof(*conn));
  memcpy(conn->path, TMP_NAME, sizeof(TMP_NAME));
  conn->c.fd = mkstemp(conn->path);
  if (conn->c.fd == -1) {
    perror("failed to open tmpfile");
    return 1;
  }
  ssize_t len = (ssize_t) strlen(s);
  if (write(conn->c.fd, s, len) != len) {
    perror("failed to write to tmpfile");
    return 1;
  }
  if (lseek(conn->c.fd, 0, SEEK_SET) == -1) {
    perror("failed to lseek tmpfile");
    return 1;
  }
  return 0;
}

int str_conn_close(StrConnection *conn) {
  if (unlink(conn->path) == -1) {
    perror("failed to unlink tmpfile");
    return 1;
  }
  if (close(conn->c.fd) == -1) {
    perror("failed to close tmpfile");
    return 1;
  }
  return 0;
}

void test_http_conn_next(void) {
  StrConnection conn;
  EXPECT_EQ(str_conn_open(&conn, "hello world\r\nhello\r\n\r\n"), 0);

  EXPECT_EQ_STR(http_conn_next(&conn.c), "hello world");
  EXPECT_EQ_STR(http_conn_next(&conn.c), "hello");
  EXPECT_EQ_STR(http_conn_next(&conn.c), "");
  EXPECT_EQ(http_conn_next(&conn.c), NULL);

  EXPECT_EQ(str_conn_close(&conn), 0);
}

TEST_MAIN(ADD_TEST(test_http_conn_next))
