#ifndef WEBB_H
#define WEBB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief An HTTP method. */
typedef enum {
  WEBB_CONNECT,
  WEBB_DELETE,
  WEBB_GET,
  WEBB_HEAD,
  WEBB_OPTIONS,
  WEBB_PATCH,
  WEBB_POST,
  WEBB_PUT,
  WEBB_TRACE,
  WEBB_INVALID = 0,
} WebbMethod;

/** @brief A list of HTTP headers. */
typedef struct WebbHeaders {
  /** @brief The key/name of the header. */
  char *key;
  /** @brief The value of the header. */
  char *val;
  /** @brief The next header in the linked list. */
  struct WebbHeaders *next;
} WebbHeaders;

/** @brief A Webb HTTP request. */
typedef struct {
  /** @brief The HTTP verb of the request. */
  WebbMethod method;
  /** @brief The uri component of the request. */
  char *uri;
  /** @brief The query string of the request (may be NULL). */
  char *query;
  /** @brief The HTTP request headers. */
  WebbHeaders *headers;
  /** @brief The HTTP request body (may be NULL). */
  char *body;
  /** @brief The length of the request body. */
  size_t body_len;
} WebbRequest;

/** @brief A Webb HTTP response. */
typedef struct {
  /** @brief HTTP status response code (e.g 200 for OK). */
  int status;
  /** @brief HTTP headers of the response. */
  WebbHeaders *headers;
  /** @brief The HTTP body (may be NULL). */
  char *body;
  /** @brief The length of the HTTP response body. */
  size_t body_len;
} WebbResponse;

/** @brief A Webb HTTP handler function. Returns zero on success. */
typedef int(WebbHandler)(const WebbRequest *, WebbResponse *);

/** @brief A Webb HTTP server. */
typedef struct {
  int sockfd;
} WebbServer;

/**
 * @brief Initalizes the server to listen to the given port.
 *
 * @param server The server to initialize.
 * @param port The port to listen to (e.g "8080").
 *
 * @returns 0 if ok, otherwise an error.
 */
int webb_server_init(WebbServer *server, const char *port);

/**
 * @brief Starts the Webb http server.
 *
 * @param server The Webb http server. Has to be successfully initialized using `webb_server_init`.
 * @param handler The http request/response handler function.
 *
 * @returns A non-zero error. Note that this function never returns unless an error occurred.
 */
int webb_server_run(WebbServer *server, WebbHandler *handler);

/**
 * @brief Get the value of a given header from the request.
 *
 * @param req The HTTP request.
 * @param key The HTTP header name.
 *
 * @returns The value of the HTTP header if present, otherwise NULL. Value is still owned by req.
 */
const char *webb_get_header(const WebbRequest *req, const char *key);

/**
 * @brief Set a given response header.
 *
 * @param res The HTTP response.
 * @param key The header name. Has to be a non-allocated string.
 * @param val The header value. Has to be an allocated string (will be freed).
 */
void webb_set_header(WebbResponse *res, char *key, char *val);

/**
 * @brief Convert an HTTP method to it's string representation (e.g HTTP_GET -> "GET").
 *
 * @param m The HTTP method.
 *
 * @returns The method string. Should not be freed.
 */
const char *webb_method_str(WebbMethod m);

/**
 * @brief Get the status message of a HTTP status code (e.g 200 -> "OK").
 *
 * @param status The HTTP status code.
 *
 * @returns The status message, or NULL if status is not a valid HTTP status code. Should not be freed.
 */
const char *webb_status_str(int status);

#ifdef __cplusplus
}
#endif

#endif
