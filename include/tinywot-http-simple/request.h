/**
 * \internal
 * \file request.h
 * \brief HTTP request related API definitions.
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
int tinywot_http_simple_extract_request_line(const char *linebuf, char *pathbuf,
                                             size_t pathbuf_size,
                                             TinyWoTRequest *request);

#ifdef __cplusplus
}
#endif
