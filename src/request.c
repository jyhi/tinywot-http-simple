/**
 * \file request.c
 * \brief HTTP request related API implementations.
 *
 * \copyright
 * SPDX-FileCopyrightText: 2021 Junde Yhi <junde@yhi.moe>
 * SPDX-License-Identifier: MIT
 */

#include <stddef.h>
#include <string.h>
#include <tinywot.h>

#include "request.h"

int tinywot_http_simple_extract_request_line(const char *linebuf, char *pathbuf,
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
