#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "http.h"
#include "server.h"
#include "test.h"

#define TMP_NAME "/tmp/webc-XXXXXX"

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

int str_conn_close(StrConnection *conn) {
  CHECK(unlink(conn->path) != -1, "failed to unlink tmpfile");
  CHECK(close(conn->c.fd) != -1, "failed to close tmpfile");
  return 0;
}

TEST(test_http_conn_next) {
  StrConnection conn;
  ASSERT_EQ(str_conn_open(&conn, "hello world\r\nhello\r\n\r\n"), 0);

  EXPECT_EQ_STR(http_conn_next(&conn.c), "hello world");
  EXPECT_EQ_STR(http_conn_next(&conn.c), "hello");
  EXPECT_EQ_STR(http_conn_next(&conn.c), "");
  EXPECT_EQ(http_conn_next(&conn.c), NULL);

  ASSERT_EQ(str_conn_close(&conn), 0);
}

TEST(test_http_parse) {
  const char *request =
    "GET /test/a.txt?abc=2 HTTP/1.1\r\n"
    "Host: localhost:8080\r\n"
    "User-Agent: curl/7.77.0\r\n"
    "Accept: */*\r\n"
    "\r\n";
  StrConnection conn;
  ASSERT_EQ(str_conn_open(&conn, request), 0);

  HttpRequest req = {0};
  ASSERT_EQ(http_parse_req(&conn.c, &req), 0);
  EXPECT_EQ(req.method, HTTP_GET);
  EXPECT_EQ_STR(req.uri, "/test/a.txt");
  EXPECT_EQ_STR(req.query, "abc=2");
  EXPECT_EQ_STR(http_get_header(req.headers, "host"), "localhost:8080");
  EXPECT_EQ_STR(http_get_header(req.headers, "user-agent"), "curl/7.77.0");
  EXPECT_EQ_STR(http_get_header(req.headers, "accept"), "*/*");
  http_req_free(&req);

  ASSERT_EQ(str_conn_close(&conn), 0);
}

TEST(test_if_one_is_one) {
  EXPECT_EQ(1, 1);
  EXPECT_EQ(1, 1);
}

TEST_MAIN(test_http_conn_next, test_http_parse, test_if_one_is_one)
