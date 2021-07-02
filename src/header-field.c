/**
 * \file header-field.c
 * \brief HTTP header field related API implementations.
 *
 * \copyright
 * SPDX-FileCopyrightText: 2021 Junde Yhi <junde@yhi.moe>
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <tinywot.h>

#include "util.h"

#include "header-field.h"

int tinywot_http_simple_extract_header_field(const char *linebuf,
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
