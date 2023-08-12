#ifndef LIBTEST_H
#define LIBTEST_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define FILENAME (strrchr("/" __FILE__, '/') + 1)

#define ERROR(assert, fmt, ...)                                                   \
  do {                                                                            \
    (void) fprintf(stderr, "%s:%d - " fmt "\n", FILENAME, __LINE__, __VA_ARGS__); \
    *test_failed = 1;                                                             \
    if (assert)                                                                   \
      return;                                                                     \
  } while (0)

#define INTERNAL_CMP(a, op, b, assert)                  \
  do {                                                  \
    if (!((a) op(b)))                                   \
      ERROR(assert, "expected: %s " #op " %s", #a, #b); \
  } while (0)

#define EXPECT_EQ(a, b) INTERNAL_CMP(a, ==, b, 0)
#define ASSERT_EQ(a, b) INTERNAL_CMP(a, ==, b, 1)

#define EXPECT_NE(a, b) INTERNAL_CMP(a, !=, b, 0)
#define ASSERT_NE(a, b) INTERNAL_CMP(a, !=, b, 1)

#define EXPECT_LT(a, b) INTERNAL_CMP(a, <, b, 0)
#define ASSERT_LT(a, b) INTERNAL_CMP(a, <, b, 1)

#define EXPECT_LE(a, b) INTERNAL_CMP(a, <=, b, 0)
#define ASSERT_LE(a, b) INTERNAL_CMP(a, <=, b, 1)

#define EXPECT_GT(a, b) INTERNAL_CMP(a, >, b, 0)
#define ASSERT_GT(a, b) INTERNAL_CMP(a, >, b, 1)

#define EXPECT_GE(a, b) INTERNAL_CMP(a, >=, b, 0)
#define ASSERT_GE(a, b) INTERNAL_CMP(a, >=, b, 1)

#define EXPECT_EQ_STR(a, b)                           \
  do {                                                \
    const char *aa = (a), *bb = (b);                  \
    if (strcmp(aa, bb) != 0)                          \
      ERROR(0, "expected: \"%s\" == \"%s\"", aa, bb); \
  } while (0)

typedef struct {
  void (*fn)(int *);
  char *name;
} TestInfo;

#define TEST(name)                                 \
  static void name##_fn(int *);                    \
  static const TestInfo name = {name##_fn, #name}; \
  static void name##_fn(int *test_failed)

#define TEST_MAIN(...)                                                              \
  int main(void) {                                                                  \
    const TestInfo tests[] = {__VA_ARGS__};                                         \
    int failed_tests = 0;                                                           \
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {                 \
      int test_failed = 0;                                                          \
      tests[i].fn(&test_failed);                                                    \
      (void) printf("%s - %s\n", test_failed ? "FAILED" : "PASSED", tests[i].name); \
      failed_tests += test_failed;                                                  \
    }                                                                               \
    (void) printf("%s: %s\n", FILENAME, failed_tests ? "FAILED" : "PASSED");        \
    return failed_tests ? 1 : 0;                                                    \
  }

#endif
