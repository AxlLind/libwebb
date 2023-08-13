#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "libtest.h"
#include "webb/webb.h"

pid_t start_child_server(WebbHandler *handler) {
  pid_t pid = fork();
  if (pid == 0)
    exit(webb_server_run("9898", handler));
  if (pid == -1)
    perror("fork");
  return pid;
}

int open_webb_socket(WebbHandler *handler, pid_t *pid) {
  *pid = start_child_server(handler);
  if (*pid == -1)
    return -1;
  struct addrinfo hints = {.ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM};
  struct addrinfo *servinfo;

  // try multiple times to give server time to start
  for (int i = 0; i < 1000; i++) {
    if (getaddrinfo("localhost", "9898", &hints, &servinfo) != 0) {
      perror("getaddrinfo");
      return -1;
    }
    int sockfd;
    struct addrinfo *p = servinfo;
    for (; p; p = p->ai_next) {
      sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
      if (sockfd == -1) {
        perror("socket");
        continue;
      }
      if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
        close(sockfd);
        continue;
      }
      break;
    }
    freeaddrinfo(servinfo);
    if (!p)
      continue;
    return sockfd;
  }
  (void) fprintf(stderr, "failed to connect to server\n");
  return -1;
}

int test_handler(const WebbRequest *req, WebbResponse *res) {
  (void) req;
  webb_set_header(res, "x-libwebb-test", strdup("cool value"));
  return 200;
}

TEST(test_sending_minimal_request) {
  pid_t pid;
  int fd = open_webb_socket(test_handler, &pid);
  ASSERT_NE(fd, -1);
  ASSERT_NE(pid, -1);

  const char *request = "GET / HTTP/1.1\r\n\r\n";
  EXPECT_NE(send(fd, request, strlen(request), 0), -1);

  char res[4096];
  ssize_t nread = read(fd, res, sizeof(res));
  EXPECT_GT(nread, 17);
  EXPECT_LT(nread, (ssize_t) sizeof(res));
  res[nread] = '\0';
  EXPECT_EQ(memcmp(res, "HTTP/1.1 200 OK\r\n", 17), 0);
  EXPECT(strstr(res, "x-libwebb-test: cool value\r\n"));
  EXPECT(strstr(res, "server: libwebb 0.1\r\n"));
  EXPECT(strstr(res, "date: "));
  EXPECT(strstr(res, "connection: "));

  EXPECT_NE(close(fd), -1);
  ASSERT_NE(kill(pid, SIGKILL), -1);
}

TEST_MAIN(test_sending_minimal_request)
