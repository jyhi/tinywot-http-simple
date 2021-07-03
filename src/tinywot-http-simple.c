/**
 * \internal
 * \file api.c
 * \brief Public API implementations.
 *
 * \copyright
 * SPDX-FileCopyrightText: 2021 Junde Yhi <junde@yhi.moe>
 * SPDX-License-Identifier: MIT
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tinywot.h>

#include "tinywot-http-simple.h"

//////////////////// Private Data ////////////////////

#ifdef TINYWOT_HTTP_SIMPLE_USE_REASON_PHRASE
#define HTTP_REASON_PHRASE_OK "OK"
#define HTTP_REASON_PHRASE_BAD_REQUEST "Bad Request"
#define HTTP_REASON_PHRASE_NOT_FOUND "Not Found"
#define HTTP_REASON_PHRASE_METHOD_NOT_ALLOWED "Method Not Allowed"
#define HTTP_REASON_PHRASE_INTERNAL_SERVER_ERROR "Internal Server Error"
#define HTTP_REASON_PHRASE_NOT_IMPLEMENTED "Not Implemented"
#else
#define HTTP_REASON_PHRASE_OK ""
#define HTTP_REASON_PHRASE_BAD_REQUEST ""
#define HTTP_REASON_PHRASE_NOT_FOUND ""
#define HTTP_REASON_PHRASE_METHOD_NOT_ALLOWED ""
#define HTTP_REASON_PHRASE_INTERNAL_SERVER_ERROR ""
#define HTTP_REASON_PHRASE_NOT_IMPLEMENTED ""
#endif

static const char crlf[] = "\r\n";

static const char ok[] = "HTTP/1.1 200 " HTTP_REASON_PHRASE_OK "\r\n";
static const char bad_request[] =
  "HTTP/1.1 400 " HTTP_REASON_PHRASE_BAD_REQUEST "\r\n";
static const char not_found[] =
  "HTTP/1.1 404 " HTTP_REASON_PHRASE_NOT_FOUND "\r\n";
static const char method_not_allowed[] =
  "HTTP/1.1 405 " HTTP_REASON_PHRASE_METHOD_NOT_ALLOWED "\r\n";
static const char internal_server_error[] =
  "HTTP/1.1 500 " HTTP_REASON_PHRASE_INTERNAL_SERVER_ERROR "\r\n";
static const char not_implemented[] =
  "HTTP/1.1 501 " HTTP_REASON_PHRASE_NOT_IMPLEMENTED "\r\n";

static const char str_content_type[] = "Content-Type: ";
static const char str_content_length[] = "Content-Length: ";

static const char text_plain[] = "text/plain\r\n";
static const char application_octet_stream[] = "application/octet-stream\r\n";
static const char application_json[] = "application/json\r\n";
static const char application_td_json[] = "application/td+json\r\n";

//////////////////// Private APIs ////////////////////

/**
 * \internal
 * \brief Return 0 if `stmt` is 0.
 */
#define RETURN_IF_FAIL(stmt) \
  { \
    int r = (stmt); \
    if (!r) \
      return 0; \
  }

/**
 * \internal
 * \brief Test up to `count` bytes if two strings are case-insensitively equal.
 *
 * \param[in] s1 A string.
 * \param[in] s2 Another string.
 * \param[in] count Up to how many bytes to compare.
 * \return non-zero if `s1[:count] == s2[:count]`, otherwise 0.
 */
static int strinequ(const char *s1, const char *s2, size_t count) {
  for (;; ++s1, ++s2, --count) {
    if (!count || (!*s1 && !*s2)) {
      return 1;
    }

    if (tolower(*s1) != tolower(*s2)) {
      return 0;
    }
  }

  return 0;
}

/**
 * \internal
 * \brief Extract useful information from a HTTP request line.
 *
 * `linebuf` should be a string of HTTP request line. For example:
 *
 * ```
 * GET /test HTTP/1.1\r\n\0
 * ```
 *
 * This function matches on the three components and store them in `request`.
 * `pathbuf` is used to copy out the path component from `linebuf`.
 *
 * \param[in] linebuf Buffer storing the current HTTP line.
 * \param[out] pathbuf Buffer storing the path component.
 * \param[in] pathbuf_size Size of `pathbuf`.
 * \param[out] request TinyWoT request representation.
 * \return non-0 if all information has been successfully matched and stored
 * into `request`, otherwise 0.
 */
static int tinywot_http_simple_extract_request_line(const char *linebuf,
                                                    char *pathbuf,
                                                    size_t pathbuf_size,
                                                    TinyWoTRequest *request) {
  const char *cursor_start = linebuf;
  const char *cursor_end = NULL;
  size_t cursor_range = 0;

  // Method

  cursor_end = strchr(cursor_start, ' ');
  if (!cursor_end) {
    return 0;
  }
  cursor_range = cursor_end - cursor_start;

  if (strncmp("GET", cursor_start, cursor_range) == 0) {
    request->op = WOT_OPERATION_TYPE_READ_PROPERTY;
  } else if (strncmp("PUT", cursor_start, cursor_range) == 0) {
    request->op = WOT_OPERATION_TYPE_WRITE_PROPERTY;
  } else if (strncmp("POST", cursor_start, cursor_range) == 0) {
    request->op = WOT_OPERATION_TYPE_INVOKE_ACTION;
  } else {
    return 0;
  }

  cursor_start = cursor_end + 1;

  // Path

  cursor_end = strchr(cursor_start, ' ');
  if (!cursor_end) {
    return 0;
  }
  cursor_range = cursor_end - cursor_start;

  if (pathbuf_size < cursor_range + 1) {
    return 0;
  }
  strncpy(pathbuf, cursor_start, cursor_range);

  cursor_start = cursor_end + 1;

  // Version (but only an assertion on the format)

  if (strncmp("HTTP/", cursor_start, 5) != 0)
    return 0;
  cursor_end = strchr(cursor_start, '\r');
  if (!cursor_end)
    return 0;
  if (*(cursor_end + 1) != '\n')
    return 0;

  return 1;
}

/**
 * \internal
 * \brief Extract useful information from a line of HTTP header field.
 *
 * `linebuf` should be a string of HTTP line of header. For example:
 *
 * ```
 * Content-Type: application/json\r\n\0
 * ```
 *
 * Header key is matched case-insensitively. On supported header key, this
 * function writes information to `request`. Currently supported header fields
 * include:
 *
 * - `content-type` => `request->content_type`
 * - `content-length` => `request->content_length`
 *
 * \param[in] linebuf Line buffer storing the current HTTP header field.
 * \param[out] request A TinyWoT request representation.
 * \return
 * - 1 if we don't care about the current header. The caller should re-fill
 *   lienbuf with the next HTTP header field and call this function again.
 * - 0 if `linebuf` only contains CR LF (`\r\n`), indicating the end of HTTP
 *   header field. The caller should stop re-filling `linebuf`.
 * - -1 if `linebuf` is a malformed HTTP header field, which is an error.
 */
static int tinywot_http_simple_extract_header_field(const char *linebuf,
                                                    TinyWoTRequest *request) {
  const char *key_start = NULL;
  const char *key_end = NULL;
  const char *value_start = NULL;
  const char *value_end = NULL;
  size_t key_length = 0;
  size_t value_length = 0;

  // If the current line consists (starts with, even) only CR and LF then we
  // indicate the over of HTTP header fields.
  if (strncmp("\r\n", linebuf, 2) == 0) {
    return 0;
  }

  // Locate key
  key_start = linebuf;
  key_end = strchr(key_start, ':');
  if (!key_end) {
    return -1;
  }
  key_length = key_end - key_start;

  // Locate value
  value_start = key_end + 1;
  value_end = strchr(value_start, '\r');
  if (!value_end || *(value_end + 1) != '\n') {
    return -1;
  }

  // Trim any optional whitespace around the value
  value_start += strspn(value_start, " \t");
  while (*(value_end - 1) == ' ' || *(value_end - 1) == '\t') {
    value_end -= 1;
  }
  value_length = value_end - value_start;

  if (strinequ("content-type", key_start, key_length)) {
    if (strinequ("text/plain", value_start, value_length)) {
      request->content_type = TINYWOT_CONTENT_TYPE_TEXT_PLAIN;
    } else if (strinequ("application/octet-stream", value_start,
                        value_length)) {
      request->content_type = TINYWOT_CONTENT_TYPE_OCTET_STREAM;
    } else if (strinequ("application/json", value_start, value_length)) {
      request->content_type = TINYWOT_CONTENT_TYPE_JSON;
    } else if (strinequ("application/td+json", value_start, value_length)) {
      request->content_type = TINYWOT_CONTENT_TYPE_TD_JSON;
    } else {
      request->content_type = TINYWOT_CONTENT_TYPE_UNKNOWN;
    }
  } else if (strinequ("content-length", key_start, key_length)) {
    unsigned long val = strtoul(value_start, NULL, 10);
    if (errno == ERANGE) {
      return -1;
    }
    request->content_length = (size_t)val;
  }

  // Now we don't care about this header field -- indicate a continuance
  return 1;
}

//////////////////// Public APIs ////////////////////

int tinywot_http_simple_recv(TinyWoTHTTPSimpleConfig *config,
                             TinyWoTRequest *request) {
  int r = 0;

  // Process HTTP request line
  r = config->readln(config->linebuf, config->linebuf_size, config->ctx);
  if (r != 1) {
    return 0;
  }
  r = tinywot_http_simple_extract_request_line(config->linebuf, config->pathbuf,
                                               config->pathbuf_size, request);
  if (!r) {
    return 0;
  }
  request->path = config->pathbuf;

  // Process HTTP header fields
  for (;;) {
    r = config->readln(config->linebuf, config->linebuf_size, config->ctx);
    if (r != 1) {
      return 0;
    }
    r = tinywot_http_simple_extract_header_field(config->linebuf, request);
    if (r < 0) {
      return 0;
    }
    if (r == 0) {
      break;
    }
    // Implied
    // if (r > 0) {
    //   continue;
    // }
  }

  // Additionally load up to linebuf_size bytes of content
  r = config->readln(config->linebuf, config->linebuf_size, config->ctx);
  if (r == -2) {
    return 0;
  }
  request->content = config->linebuf;

  return 1;
}

int tinywot_http_simple_send(TinyWoTHTTPSimpleConfig *config,
                             TinyWoTResponse *response) {
  // HTTP status line
  switch (response->status) {
    case TINYWOT_RESPONSE_STATUS_OK:
      RETURN_IF_FAIL(config->write(ok, sizeof(ok), config->ctx));
      break;
    case TINYWOT_RESPONSE_STATUS_BAD_REQUEST:
      RETURN_IF_FAIL(
        config->write(bad_request, sizeof(bad_request), config->ctx));
      break;
    case TINYWOT_RESPONSE_STATUS_UNSUPPORTED:
      RETURN_IF_FAIL(config->write(not_found, sizeof(not_found), config->ctx));
      break;
    case TINYWOT_RESPONSE_STATUS_METHOD_NOT_ALLOWED:
      RETURN_IF_FAIL(config->write(method_not_allowed,
                                   sizeof(method_not_allowed), config->ctx));
      break;
    case TINYWOT_RESPONSE_STATUS_NOT_IMPLEMENTED:
      RETURN_IF_FAIL(
        config->write(not_implemented, sizeof(not_implemented), config->ctx));
      break;
    case TINYWOT_RESPONSE_STATUS_ERROR:   // fall through
    case TINYWOT_RESPONSE_STATUS_UNKNOWN: // fall through
    default:
      RETURN_IF_FAIL(config->write(internal_server_error,
                                   sizeof(internal_server_error), config->ctx));
      break;
  }

  // If there is actually no content payload, then we stop here
  if (!response->content) {
    RETURN_IF_FAIL(config->write(crlf, sizeof(crlf), config->ctx));
    return 1;
  }

  // Content-Type
  RETURN_IF_FAIL(
    config->write(str_content_type, sizeof(str_content_type), config->ctx));

  switch (response->content_type) {
    case TINYWOT_CONTENT_TYPE_OCTET_STREAM:
      RETURN_IF_FAIL(config->write(application_octet_stream,
                                   sizeof(application_octet_stream),
                                   config->ctx));
      break;
    case TINYWOT_CONTENT_TYPE_JSON:
      RETURN_IF_FAIL(
        config->write(application_json, sizeof(application_json), config->ctx));
      break;
    case TINYWOT_CONTENT_TYPE_TD_JSON:
      RETURN_IF_FAIL(config->write(application_td_json,
                                   sizeof(application_td_json), config->ctx));
      break;
    case TINYWOT_CONTENT_TYPE_TEXT_PLAIN: // fall through
    case TINYWOT_CONTENT_TYPE_UNKNOWN:    // fall through
    default:
      RETURN_IF_FAIL(
        config->write(text_plain, sizeof(text_plain), config->ctx));
      break;
  }

  // Content-Length
  int nbytes = snprintf(config->linebuf, config->linebuf_size, "%u",
                        response->content_length);
  RETURN_IF_FAIL(
    config->write(str_content_length, sizeof(str_content_length), config->ctx));
  RETURN_IF_FAIL(config->write(config->linebuf, (size_t)nbytes, config->ctx));
  RETURN_IF_FAIL(config->write(crlf, sizeof(crlf), config->ctx));

  // End of header
  RETURN_IF_FAIL(config->write(crlf, sizeof(crlf), config->ctx));

  // Content payload
  RETURN_IF_FAIL(
    config->write(response->content, response->content_length, config->ctx));

  return 1;
}
