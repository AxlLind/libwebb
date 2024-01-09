#ifndef LIBTEST_H
#define LIBTEST_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define FILENAME (strrchr("/" __FILE__, '/') + 1)

#define INTERNAL_ASSERT(expr, assert)                              \
  do {                                                             \
    if (!(expr)) {                                                 \
      printf("%s:%d - expected: %s\n", FILENAME, __LINE__, #expr); \
      *test_failed = 1;                                            \
      if (assert)                                                  \
        return;                                                    \
    }                                                              \
  } while (0)

#define EXPECT(expr) INTERNAL_ASSERT(expr, 0)
#define ASSERT(expr) INTERNAL_ASSERT(expr, 1)

typedef struct {
  void (*fn)(int *);
  char *name;
} TestInfo;

#define TEST(name)                                 \
  static void name##_fn(int *);                    \
  static const TestInfo name = {name##_fn, #name}; \
  static void name##_fn(int *test_failed)

#define TEST_MAIN(...)                                                      \
  int main(void) {                                                          \
    const TestInfo tests[] = {__VA_ARGS__};                                 \
    int failed_tests = 0;                                                   \
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {         \
      int test_failed = 0;                                                  \
      tests[i].fn(&test_failed);                                            \
      failed_tests += test_failed;                                          \
      printf("%s: %s\n", test_failed ? "FAILED" : "PASSED", tests[i].name); \
    }                                                                       \
    return failed_tests ? 1 : 0;                                            \
  }

#endif
