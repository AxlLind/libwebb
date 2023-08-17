#ifndef TMPFILE_H
#define TMPFILE_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "internal.h"
#include "webb/webb.h"

#define TMP_NAME "/tmp/libwebb-XXXXXX"

#define CHECK(ok, msg) \
  do {                 \
    if (!(ok)) {       \
      perror(msg);     \
      return 1;        \
    }                  \
  } while (0)

typedef struct TmpFile {
  int fd;
  char path[sizeof(TMP_NAME)];
} TmpFile;

int tmpfile_open(TmpFile *f, const char *s) {
  memset(f, 0, sizeof(*f));
  memcpy(f->path, TMP_NAME, sizeof(TMP_NAME));
  ssize_t len = (ssize_t) strlen(s);
  f->fd = mkstemp(f->path);
  CHECK(f->fd != -1, "failed to open tmpfile");
  CHECK(write(f->fd, s, len) == len, "failed to write to tmpfile");
  CHECK(lseek(f->fd, 0, SEEK_SET) != -1, "failed to lseek tmpfile");
  return 0;
}

int tmpfile_reopen(TmpFile *f, const char *s) {
  ssize_t len = (ssize_t) strlen(s);
  CHECK(ftruncate(f->fd, 0) != -1, "failed to truncate tmpfile");
  CHECK(pwrite(f->fd, s, len, 0) == len, "failed to write to tmpfile");
  CHECK(lseek(f->fd, 0, SEEK_SET) != -1, "failed to lseek tmpfile");
  return 0;
}

int tmpfile_close(TmpFile *f) {
  CHECK(unlink(f->path) != -1, "failed to unlink tmpfile");
  CHECK(close(f->fd) != -1, "failed to close tmpfile");
  return 0;
}

#endif
