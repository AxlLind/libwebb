#include <stdio.h>
#include "internal.h"
#include "libtest.h"
#include "str_conn.h"
#include "webb/webb.h"

static StrConnection conn;
static WebbRequest req;

TEST(test_parse_curl_example) {
  const char *request =
    "GET /test/a.txt?abc=2 HTTP/1.1\r\n"
    "Host: localhost:8080\r\n"
    "User-Agent: curl/7.77.0\r\n"
    "Accept: */*\r\n"
    "\r\n";
  ASSERT(str_conn_open(&conn, request) == 0);
  ASSERT(http_parse_req(&conn.c, &req) == 0);

  EXPECT(req.method == WEBB_GET);
  EXPECT_EQ_STR(req.uri, "/test/a.txt");
  EXPECT_EQ_STR(req.query, "abc=2");
  EXPECT_EQ_STR(webb_get_header(&req, "host"), "localhost:8080");
  EXPECT_EQ_STR(webb_get_header(&req, "user-agent"), "curl/7.77.0");
  EXPECT_EQ_STR(webb_get_header(&req, "accept"), "*/*");
  EXPECT(req.body == NULL);
  EXPECT(req.body_len == 0);
  http_req_free(&req);

  ASSERT(str_conn_close(&conn) == 0);
}

TEST(test_parse_minimal_request) {
  const char *request = "GET / HTTP/1.1\r\n\r\n";
  ASSERT(str_conn_open(&conn, request) == 0);
  ASSERT(http_parse_req(&conn.c, &req) == 0);

  EXPECT(req.method == WEBB_GET);
  EXPECT_EQ_STR(req.uri, "/");
  EXPECT(req.query == NULL);
  EXPECT(req.headers == NULL);
  EXPECT(req.body == NULL);
  EXPECT(req.body_len == 0);
  http_req_free(&req);

  ASSERT(str_conn_close(&conn) == 0);
}

TEST(test_parse_request_body) {
  const char *request =
    "POST / HTTP/1.1\r\n"
    "Content-Length: 11\r\n"
    "\r\n"
    "hello world";
  ASSERT(str_conn_open(&conn, request) == 0);
  ASSERT(http_parse_req(&conn.c, &req) == 0);

  EXPECT(req.method == WEBB_POST);
  EXPECT_EQ_STR(req.uri, "/");
  EXPECT(req.query == NULL);
  EXPECT_EQ_STR(webb_get_header(&req, "content-length"), "11");
  EXPECT_EQ_STR(req.body, "hello world");
  EXPECT(req.body_len == 11);
  http_req_free(&req);
}

TEST(test_missing_final_newline) {
  const char *request = "GET / HTTP/1.1\r\n";
  ASSERT(str_conn_open(&conn, request) == 0);

  EXPECT(http_parse_req(&conn.c, &req) != 0);
  http_req_free(&req);

  ASSERT(str_conn_close(&conn) == 0);
}

TEST(test_invalid_http_version) {
  ASSERT(str_conn_open(&conn, "") == 0);

#define INVALID_VERSION_TEST(version)                               \
  ASSERT(str_conn_reopen(&conn, "GET / " version "\r\n\r\n") == 0); \
  EXPECT(http_parse_req(&conn.c, &req) != 0);                       \
  http_req_free(&req)

  INVALID_VERSION_TEST("");
  INVALID_VERSION_TEST("HTTP/1.0");
  INVALID_VERSION_TEST("HTTP/2.0");
  INVALID_VERSION_TEST("http/1.1");
  INVALID_VERSION_TEST("HTTP 1.1");
  INVALID_VERSION_TEST("FTP/1.1");

  ASSERT(str_conn_close(&conn) == 0);
}

TEST(test_multiple_requests_per_connection) {
  const char *request =
    "GET / HTTP/1.1\r\n\r\n"
    "GET / HTTP/1.1\r\n\r\n"
    "GET / HTTP/1.1\r\n\r\n";
  ASSERT(str_conn_open(&conn, request) == 0);

  for (int i = 0; i < 3; i++) {
    EXPECT(http_parse_req(&conn.c, &req) == 0);
    EXPECT(req.method == WEBB_GET);
    EXPECT_EQ_STR(req.uri, "/");
    EXPECT(req.query == NULL);
    EXPECT(req.headers == NULL);
    EXPECT(req.body == NULL);
    EXPECT(req.body_len == 0);
    http_req_free(&req);
  }
  EXPECT(http_parse_req(&conn.c, &req) != 0);
  http_req_free(&req);

  ASSERT(str_conn_close(&conn) == 0);
}

TEST(test_max_header_limit) {
  char request[65536], *ptr = request;
  ptr += sprintf(ptr, "GET / HTTP/1.1\r\n");
  for (int i = 0; i < MAX_HEADERS; i++)
    ptr += sprintf(ptr, "Header-%d: Value\r\n", i);
  (void) sprintf(ptr, "\r\n");

  // should parse ok with one less than the maximum
  ASSERT(str_conn_open(&conn, request) == 0);
  EXPECT(http_parse_req(&conn.c, &req) == 0);
  http_req_free(&req);

  // should fail to parse with exactly the maximum
  (void) sprintf(ptr, "Header-%d: Value\r\n\r\n", MAX_HEADERS);
  ASSERT(str_conn_reopen(&conn, request) == 0);
  EXPECT(http_parse_req(&conn.c, &req) != 0);
  http_req_free(&req);

  ASSERT(str_conn_close(&conn) == 0);
}

TEST_MAIN(
  test_parse_curl_example,
  test_parse_minimal_request,
  test_parse_request_body,
  test_missing_final_newline,
  test_invalid_http_version,
  test_multiple_requests_per_connection,
  test_max_header_limit)
