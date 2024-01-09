# libwebb
`libwebb` (webb is web in Swedish) is a small http server framework written in pure C99 with zero dependencies (for POSIX systems).

## Usage
The basic usage of the library is as follows:

```C
#include <string.h>
#include "webb/webb.h"

int http_handler(const WebbRequest *req, WebbResponse *res) {
  // implement behavior given the HTTP request...
  if (req->method != WEBB_GET)
    return 404;
  webb_set_body_static(res, "hello world", 11);
  webb_set_header(res, "content-type", strdup("text/raw"));
  return 200;
}

int main(void) {
  // do some setup...
  return webb_server_run("8080", http_handler);
}
```

For API documentation, see the [library header file](./include/webb/webb.h). The API is fully documented using doxygen comments.

See [./bin/webb.c](./bin/webb.c) for a small web server using the framework.

## Compilation and installation
```bash
make build              # build the library
cp out/libwebb.a  $DIR  # copy the lib to your destination
cp out/libwebb.so $DIR  # or the dynamic library

# compile against the library and include the header path
gcc ... $DIR/libwebb.a  -I$LIBWEBB_DIR/include
gcc ... $DIR/libwebb.so -I$LIBWEBB_DIR/include
```

## Development
`libwebb` uses `make` as it's build system. The makefile is self-documenting. Simply type `make` to get help about available commands to use to build, lint, and test during development.
