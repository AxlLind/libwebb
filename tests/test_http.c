#include <stdio.h>
#include "internal.h"
#include "libtest.h"
#include "tmpfile.h"
#include "webb/webb.h"

static TmpFile TMPFILE;
static HttpParseState STATE;
static WebbRequest REQ;

static int open_request(const char *request) {
  if (tmpfile_open(&TMPFILE, request) != 0)
    return 1;
  memset(&STATE, 0, sizeof(STATE));
  return 0;
}

static int reopen_request(const char *request) {
  if (tmpfile_reopen(&TMPFILE, request) != 0)
    return 1;
  memset(&STATE, 0, sizeof(STATE));
  return 0;
}

TEST(test_parse_curl_example) {
  const char *request =
    "GET /test/a.txt?abc=2 HTTP/1.1\r\n"
    "Host: localhost:8080\r\n"
    "User-Agent: curl/7.77.0\r\n"
    "Accept: */*\r\n"
    "\r\n";
  ASSERT(open_request(request) == 0);
  ASSERT(parse_request(TMPFILE.fd, &STATE, &REQ) == RESULT_OK);

  EXPECT(REQ.method == WEBB_GET);
  EXPECT(strcmp(REQ.uri, "/test/a.txt") == 0);
  EXPECT(strcmp(REQ.query, "abc=2") == 0);
  EXPECT(strcmp(webb_get_header(&REQ, "host"), "localhost:8080") == 0);
  EXPECT(strcmp(webb_get_header(&REQ, "user-agent"), "curl/7.77.0") == 0);
  EXPECT(strcmp(webb_get_header(&REQ, "accept"), "*/*") == 0);
  EXPECT(REQ.body == NULL);
  EXPECT(REQ.body_len == 0);
  http_req_free(&REQ);

  ASSERT(tmpfile_close(&TMPFILE) == 0);
}

TEST(test_parse_minimal_request) {
  const char *request = "GET / HTTP/1.1\r\n\r\n";
  ASSERT(open_request(request) == 0);
  ASSERT(parse_request(TMPFILE.fd, &STATE, &REQ) == RESULT_OK);

  EXPECT(REQ.method == WEBB_GET);
  EXPECT(strcmp(REQ.uri, "/") == 0);
  EXPECT(REQ.query == NULL);
  EXPECT(REQ.headers == NULL);
  EXPECT(REQ.body == NULL);
  EXPECT(REQ.body_len == 0);
  http_req_free(&REQ);

  ASSERT(tmpfile_close(&TMPFILE) == 0);
}

TEST(test_parse_request_body) {
  const char *request =
    "POST / HTTP/1.1\r\n"
    "Content-Length: 11\r\n"
    "\r\n"
    "hello world";
  ASSERT(open_request(request) == 0);
  ASSERT(parse_request(TMPFILE.fd, &STATE, &REQ) == RESULT_OK);

  EXPECT(REQ.method == WEBB_POST);
  EXPECT(strcmp(REQ.uri, "/") == 0);
  EXPECT(REQ.query == NULL);
  EXPECT(strcmp(webb_get_header(&REQ, "content-length"), "11") == 0);
  EXPECT(strcmp(REQ.body, "hello world") == 0);
  EXPECT(REQ.body_len == 11);
  http_req_free(&REQ);
}

TEST(test_missing_final_newline) {
  const char *request = "GET / HTTP/1.1\r\n";
  ASSERT(open_request(request) == 0);

  EXPECT(parse_request(TMPFILE.fd, &STATE, &REQ) != RESULT_OK);
  http_req_free(&REQ);

  ASSERT(tmpfile_close(&TMPFILE) == 0);
}

TEST(test_invalid_http_version) {
  ASSERT(open_request("") == 0);

#define INVALID_VERSION_TEST(version)                           \
  ASSERT(reopen_request("GET / " version "\r\n\r\n") == 0);     \
  EXPECT(parse_request(TMPFILE.fd, &STATE, &REQ) != RESULT_OK); \
  http_req_free(&REQ);

  INVALID_VERSION_TEST("");
  INVALID_VERSION_TEST("HTTP/1.0");
  INVALID_VERSION_TEST("HTTP/2.0");
  INVALID_VERSION_TEST("http/1.1");
  INVALID_VERSION_TEST("HTTP 1.1");
  INVALID_VERSION_TEST("FTP/1.1");

  ASSERT(tmpfile_close(&TMPFILE) == 0);
}

TEST(test_multiple_requests_per_connection) {
  const char *request =
    "GET / HTTP/1.1\r\n\r\n"
    "GET / HTTP/1.1\r\n\r\n"
    "GET / HTTP/1.1\r\n\r\n";
  ASSERT(open_request(request) == 0);

  for (int i = 0; i < 3; i++) {
    EXPECT(parse_request(TMPFILE.fd, &STATE, &REQ) == RESULT_OK);
    EXPECT(REQ.method == WEBB_GET);
    EXPECT(strcmp(REQ.uri, "/") == 0);
    EXPECT(REQ.query == NULL);
    EXPECT(REQ.headers == NULL);
    EXPECT(REQ.body == NULL);
    EXPECT(REQ.body_len == 0);
    http_req_free(&REQ);
    http_state_reset(&STATE);
  }
  EXPECT(parse_request(TMPFILE.fd, &STATE, &REQ) != RESULT_OK);
  http_req_free(&REQ);

  ASSERT(tmpfile_close(&TMPFILE) == 0);
}

TEST(test_max_header_limit) {
  char request[4096], *ptr = request;
  ptr += sprintf(ptr, "GET / HTTP/1.1\r\n");
  for (int i = 0; i < MAX_HEADERS; i++)
    ptr += sprintf(ptr, "Header-%d: Value\r\n", i);
  (void) sprintf(ptr, "\r\n");

  // should parse ok with one less than the maximum
  ASSERT(open_request(request) == 0);
  EXPECT(parse_request(TMPFILE.fd, &STATE, &REQ) == RESULT_OK);
  http_req_free(&REQ);

  // should fail to parse with exactly the maximum
  ptr += sprintf(ptr, "Header-%d: Value\r\n\r\n", MAX_HEADERS);
  ASSERT(ptr < request + sizeof(request));
  EXPECT(reopen_request(request) == 0);
  EXPECT(parse_request(TMPFILE.fd, &STATE, &REQ) != RESULT_OK);
  http_req_free(&REQ);

  ASSERT(tmpfile_close(&TMPFILE) == 0);
}

TEST_MAIN(
  test_parse_curl_example,
  test_parse_minimal_request,
  test_parse_request_body,
  test_missing_final_newline,
  test_invalid_http_version,
  test_multiple_requests_per_connection,
  test_max_header_limit)
