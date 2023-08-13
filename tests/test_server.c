#include "internal.h"
#include "libtest.h"
#include "str_conn.h"
#include "webb/webb.h"

static StrConnection conn;

TEST(test_http_conn_next) {
  ASSERT(str_conn_open(&conn, "hello world\r\nhello\r\n\r\n") == 0);

  EXPECT_EQ_STR(http_conn_next(&conn.c), "hello world");
  EXPECT_EQ_STR(http_conn_next(&conn.c), "hello");
  EXPECT_EQ_STR(http_conn_next(&conn.c), "");
  EXPECT(http_conn_next(&conn.c) == NULL);

  ASSERT(str_conn_close(&conn) == 0);
}

TEST_MAIN(test_http_conn_next)
