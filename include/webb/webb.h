#ifndef WEBB_H
#define WEBB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief An HTTP method. */
typedef enum WebbMethod {
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
typedef struct WebbRequest {
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

/** @brief The type of the response body. */
typedef enum WebbBodyType {
  WEBB_BODY_NULL = 0,
  WEBB_BODY_ALLOCATED,
  WEBB_BODY_STATIC,
  WEBB_BODY_FD,
} WebbBodyType;

typedef struct WebbBody {
  /** @brief The length of the HTTP response body. */
  size_t len;
  /** @brief The body type. */
  WebbBodyType type;
  union {
    /** @brief The HTTP body buffer. */
    char *buf;
    /** @brief The HTTP body file descriptor. */
    int fd;
  } body;
} WebbBody;

/** @brief A Webb HTTP response. */
typedef struct WebbResponse {
  /** @brief HTTP status response code (e.g 200 for OK). */
  int status;
  /** @brief HTTP headers of the response. */
  WebbHeaders *headers;
  /** @brief HTTP body to send */
  WebbBody body;
} WebbResponse;

/**
 * @brief A Webb HTTP handler function. Accepts an incoming request and returns a response.
 *
 * @param req The HTTP request object.
 * @param res The HTTP response object, mutated by the function.
 *
 * @returns The HTTP status code (e.g 200 for OK), -1 on unexpected errors.
 */
typedef int(WebbHandler)(const WebbRequest *req, WebbResponse *res);

/**
 * @brief Starts the Webb http server.
 *
 * @param port    The port to listen to (e.g "8080").
 * @param handler The http request/response handler function.
 *
 * @returns A non-zero error. Note that this function never returns unless an error occurred.
 */
int webb_server_run(const char *port, WebbHandler *handler);

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
 * @brief Set the body of the response. Body will be freed.
 *
 * @param res The HTTP response.
 * @param body The body to send. Has to be allocated.
 * @param len The length of the body.
 */
void webb_set_body(WebbResponse *res, char *body, size_t len);

/**
 * @brief Set the body of the response as a static string. Body will NOT be freed.
 *
 * @param res The HTTP response.
 * @param body The body to send.
 * @param len The length of the body.
 */
void webb_set_body_static(WebbResponse *res, char *body, size_t len);

/**
 * @brief Set the body of the response as a file descriptor to send. E.g an opened file.
 *
 * @param res The HTTP response.
 * @param fd The file descriptor to read from.
 */
void webb_set_body_fd(WebbResponse *res, int fd, size_t len);

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
