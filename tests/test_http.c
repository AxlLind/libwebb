#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "internal.h"
#include "libtest.h"
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

static StrConnection conn;
static WebbRequest req;

TEST(test_http_conn_next) {
  ASSERT_EQ(str_conn_open(&conn, "hello world\r\nhello\r\n\r\n"), 0);

  EXPECT_EQ_STR(http_conn_next(&conn.c), "hello world");
  EXPECT_EQ_STR(http_conn_next(&conn.c), "hello");
  EXPECT_EQ_STR(http_conn_next(&conn.c), "");
  EXPECT_EQ(http_conn_next(&conn.c), NULL);

  ASSERT_EQ(str_conn_close(&conn), 0);
}

TEST(test_parse_curl_example) {
  const char *request =
    "GET /test/a.txt?abc=2 HTTP/1.1\r\n"
    "Host: localhost:8080\r\n"
    "User-Agent: curl/7.77.0\r\n"
    "Accept: */*\r\n"
    "\r\n";
  ASSERT_EQ(str_conn_open(&conn, request), 0);
  ASSERT_EQ(http_parse_req(&conn.c, &req), 0);

  EXPECT_EQ(req.method, WEBB_GET);
  EXPECT_EQ_STR(req.uri, "/test/a.txt");
  EXPECT_EQ_STR(req.query, "abc=2");
  EXPECT_EQ_STR(webb_get_header(&req, "host"), "localhost:8080");
  EXPECT_EQ_STR(webb_get_header(&req, "user-agent"), "curl/7.77.0");
  EXPECT_EQ_STR(webb_get_header(&req, "accept"), "*/*");
  EXPECT_EQ(req.body, NULL);
  EXPECT_EQ(req.body_len, 0);
  http_req_free(&req);

  ASSERT_EQ(str_conn_close(&conn), 0);
}

TEST(test_parse_minimal_request) {
  const char *request = "GET / HTTP/1.1\r\n\r\n";
  ASSERT_EQ(str_conn_open(&conn, request), 0);
  ASSERT_EQ(http_parse_req(&conn.c, &req), 0);

  EXPECT_EQ(req.method, WEBB_GET);
  EXPECT_EQ_STR(req.uri, "/");
  EXPECT_EQ(req.query, NULL);
  EXPECT_EQ(req.headers, NULL);
  EXPECT_EQ(req.body, NULL);
  EXPECT_EQ(req.body_len, 0);
  http_req_free(&req);

  ASSERT_EQ(str_conn_close(&conn), 0);
}

TEST(test_parse_request_body) {
  const char *request =
    "POST / HTTP/1.1\r\n"
    "Content-Length: 11\r\n"
    "\r\n"
    "hello world";
  ASSERT_EQ(str_conn_open(&conn, request), 0);
  ASSERT_EQ(http_parse_req(&conn.c, &req), 0);

  EXPECT_EQ(req.method, WEBB_POST);
  EXPECT_EQ_STR(req.uri, "/");
  EXPECT_EQ(req.query, NULL);
  EXPECT_EQ_STR(webb_get_header(&req, "content-length"), "11");
  EXPECT_EQ_STR(req.body, "hello world");
  EXPECT_EQ(req.body_len, 11);
  http_req_free(&req);
}

TEST(test_missing_final_newline) {
  const char *request = "GET / HTTP/1.1\r\n";
  ASSERT_EQ(str_conn_open(&conn, request), 0);

  EXPECT_NE(http_parse_req(&conn.c, &req), 0);
  http_req_free(&req);

  ASSERT_EQ(str_conn_close(&conn), 0);
}

TEST(test_invalid_http_version) {
  ASSERT_EQ(str_conn_open(&conn, ""), 0);

#define INVALID_VERSION_TEST(version)                                \
  ASSERT_EQ(str_conn_reopen(&conn, "GET / " version "\r\n\r\n"), 0); \
  EXPECT_NE(http_parse_req(&conn.c, &req), 0);                       \
  http_req_free(&req)

  INVALID_VERSION_TEST("");
  INVALID_VERSION_TEST("HTTP/1.0");
  INVALID_VERSION_TEST("HTTP/2.0");
  INVALID_VERSION_TEST("http/1.1");
  INVALID_VERSION_TEST("HTTP 1.1");
  INVALID_VERSION_TEST("FTP/1.1");

  ASSERT_EQ(str_conn_close(&conn), 0);
}

TEST(test_multiple_requests_per_connection) {
  const char *request =
    "GET / HTTP/1.1\r\n\r\n"
    "GET / HTTP/1.1\r\n\r\n"
    "GET / HTTP/1.1\r\n\r\n";
  ASSERT_EQ(str_conn_open(&conn, request), 0);

  for (int i = 0; i < 3; i++) {
    EXPECT_EQ(http_parse_req(&conn.c, &req), 0);
    EXPECT_EQ(req.method, WEBB_GET);
    EXPECT_EQ_STR(req.uri, "/");
    EXPECT_EQ(req.query, NULL);
    EXPECT_EQ(req.headers, NULL);
    EXPECT_EQ(req.body, NULL);
    EXPECT_EQ(req.body_len, 0);
    http_req_free(&req);
  }
  EXPECT_NE(http_parse_req(&conn.c, &req), 0);
  http_req_free(&req);

  ASSERT_EQ(str_conn_close(&conn), 0);
}

TEST(test_max_header_limit) {
  char request[65536], *ptr = request;
  ptr += sprintf(ptr, "GET / HTTP/1.1\r\n");
  for (int i = 0; i < MAX_HEADERS; i++)
    ptr += sprintf(ptr, "Header-%d: Value\r\n", i);
  (void) sprintf(ptr, "\r\n");

  // should parse ok with one less than the maximum
  ASSERT_EQ(str_conn_open(&conn, request), 0);
  EXPECT_EQ(http_parse_req(&conn.c, &req), 0);
  http_req_free(&req);

  // should fail to parse with exactly the maximum
  (void) sprintf(ptr, "Header-%d: Value\r\n\r\n", MAX_HEADERS);
  ASSERT_EQ(str_conn_reopen(&conn, request), 0);
  EXPECT_NE(http_parse_req(&conn.c, &req), 0);
  http_req_free(&req);

  ASSERT_EQ(str_conn_close(&conn), 0);
}

TEST_MAIN(
  test_http_conn_next,
  test_parse_curl_example,
  test_parse_minimal_request,
  test_parse_request_body,
  test_missing_final_newline,
  test_invalid_http_version,
  test_multiple_requests_per_connection,
  test_max_header_limit)
