# libwebb
`libwebb` (webb is web in Swedish) is a small http server framework written in pure C99 with zero dependencies (for POSIX systems).

## Usage
The basic usage of the library is as follows:

```C
#include "webb/webb.h"

int http_handler(const WebbRequest *req, WebbResponse *res) {
  // implement behavior given the HTTP request...
  res->body = strdup("hello world");
  res->body_len = strlen("hello world");
  res->status = 200;
  return 0;
}

int main(void) {
  WebbServer server;
  if (webb_server_init(&server, "8080") != 0)
    return 1;
  return webb_server_run(&server, http_handler);
}
```

For API documentation, see the [library header file](./include/webb/webb.h).

See [./bin/webb.c](./bin/webb.c) for a small web server using the framework.

## Compilation and installation
```bash
make build            # build the library
cp out/libwebb.a $DIR # copy the lib to your destination
```

Add the flag `-I$LIBWEBB_DIR/include` to be able to include the library header.
