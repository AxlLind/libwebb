#ifndef LIBTEST_H
#define LIBTEST_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define FILENAME (strrchr("/" __FILE__, '/') + 1)

#define INTERNAL_ASSERT(assert, expr, msg)                                       \
  do {                                                                           \
    if (!(expr)) {                                                               \
      (void) fprintf(stderr, "%s:%d - expected: %s\n", FILENAME, __LINE__, msg); \
      *test_failed = 1;                                                          \
      if (assert)                                                                \
        return;                                                                  \
    }                                                                            \
  } while (0)

#define EXPECT(expr) INTERNAL_ASSERT(0, expr, #expr)
#define ASSERT(expr) INTERNAL_ASSERT(1, expr, #expr)

#define EXPECT_EQ_STR(a, b) INTERNAL_ASSERT(0, strcmp((a), (b)) == 0, #a " == " #b)
#define ASSERT_EQ_STR(a, b) INTERNAL_ASSERT(1, strcmp((a), (b)) == 0, #a " == " #b)

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
    return failed_tests ? 1 : 0;                                                    \
  }

#endif
