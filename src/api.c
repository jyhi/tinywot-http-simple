/**
 * \file api.c
 * \brief Public API implementations.
 *
 * \copyright
 * SPDX-FileCopyrightText: 2021 Junde Yhi <junde@yhi.moe>
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <tinywot.h>

#include "header-field.h"
#include "request.h"
#include "type.h"
#include "util.h"

#include "api.h"

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

/**
 * \brief Return 0 if `stmt` is 0.
 */
#define RETURN_IF_FAIL(stmt) \
  { \
    int r = (stmt); \
    if (!r) \
      return 0; \
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
