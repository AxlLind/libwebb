# libwebb
`libwebb` (webb is web in Swedish) is a small http server framework written in pure C99 with zero dependencies (for POSIX systems).

## Usage
The basic usage of the library is as follows:

```C
#include "webb/webb.h"

int http_handler(const WebbRequest *req, WebbResponse *res) {
  // implement behavior given the HTTP request...
  if (req->method != WEBB_GET)
    return 404;

  res->body = strdup("hello world");
  res->body_len = strlen(res->body);
  webb_set_header(res, "content-type", strdup("text/raw"));
  return 200;
}

int main(void) {
  WebbServer server;
  if (webb_server_init(&server, "8080") != 0)
    return 1;
  return webb_server_run(&server, http_handler);
}
```

For API documentation, see the [library header file](./include/webb/webb.h). The API is fully documented using doxygen comments.

See [./bin/webb.c](./bin/webb.c) for a small web server using the framework.

## Compilation and installation
```bash
make build            # build the library
cp out/libwebb.a $DIR # copy the lib to your destination

# compile against the library and include the header path
gcc ... $DIR/libwebb.a -I$LIBWEBB_DIR/include
```
