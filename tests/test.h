#ifndef TEST_H
#define TEST_H

#include <stddef.h>
#include <stdio.h>

int g_test_status;

#define FILENAME (strrchr("/" __FILE__, '/') + 1)

#define ERROR(assert, fmt, ...)                                            \
  do {                                                                     \
    fprintf(stderr, "%s:%d - " fmt "\n", FILENAME, __LINE__, __VA_ARGS__); \
    g_test_status = 1;                                                     \
    if (assert)                                                            \
      return;                                                              \
  } while (0)

#define INTERNAL_EQ(a, b, assert)                  \
  do {                                             \
    if ((a) != (b))                                \
      ERROR(assert, "expected: %s == %s", #a, #b); \
  } while (0)

#define EXPECT_EQ(a, b) INTERNAL_EQ(a, b, 0)
#define ASSERT_EQ(a, b) INTERNAL_EQ(a, b, 1)

#define EXPECT_EQ_STR(a, b)                           \
  do {                                                \
    const char *aa = (a), *bb = (b);                  \
    if (strcmp(aa, bb) != 0)                          \
      ERROR(0, "expected: \"%s\" == \"%s\"", aa, bb); \
  } while (0)

typedef struct {
  void (*fn)(void);
  char *name;
} TestInfo;

#define TEST(name)                    \
  void name##_fn(void);               \
  TestInfo name = {name##_fn, #name}; \
  void name##_fn(void)

#define TEST_MAIN(...)                                                         \
  int main(void) {                                                             \
    TestInfo tests[] = {__VA_ARGS__};                                          \
    int status = 0;                                                            \
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {            \
      g_test_status = 0;                                                       \
      tests[i].fn();                                                           \
      printf("%s - %s\n", g_test_status ? "FAILED" : "PASSED", tests[i].name); \
      status |= g_test_status;                                                 \
    }                                                                          \
    printf("%s: %s\n", FILENAME, status ? "FAILED" : "PASSED");                \
    return status;                                                             \
  }

#endif
