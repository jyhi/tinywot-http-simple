/**
 * \internal
 * \file header-field.h
 * \brief HTTP header field related API definitions.
 *
 * \copyright
 * SPDX-FileCopyrightText: 2021 Junde Yhi <junde@yhi.moe>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <tinywot.h>

#ifdef __cplusplus
extern "C" {
#endif

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
int tinywot_http_simple_extract_header_field(const char *linebuf,
                                             TinyWoTRequest *request);

#ifdef __cplusplus
}
#endif
