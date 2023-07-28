#ifndef TEST_H
#define TEST_H

#include <stddef.h>
#include <stdio.h>

int g_test_status;

#define FILENAME (strrchr("/" __FILE__, '/') + 1)

#define ERROR_MSG(fmt, ...) fprintf(stderr, "%s:%d - " fmt "\n", FILENAME, __LINE__, __VA_ARGS__)

#define EXPECT_EQ(a, b)                       \
  do {                                        \
    if ((a) != (b)) {                         \
      ERROR_MSG("expected %s == %s", #a, #b); \
      g_test_status = 1;                      \
    }                                         \
  } while (0)

#define EXPECT_EQ_STR(a, b)                           \
  do {                                                \
    const char *aa = (a);                             \
    const char *bb = (b);                             \
    if (strcmp(aa, bb) != 0) {                        \
      ERROR_MSG("expected \"%s\" == \"%s\"", aa, bb); \
      g_test_status = 1;                              \
    }                                                 \
  } while (0)

#define ADD_TEST(test) \
  { test, #test }

#define TEST_MAIN(...)                                                         \
  int main(void) {                                                             \
    struct {                                                                   \
      void (*fn)(void);                                                        \
      char *name;                                                              \
    } tests[] = {__VA_ARGS__};                                                 \
                                                                               \
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
