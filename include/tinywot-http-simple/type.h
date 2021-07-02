/**
 * \file type.h
 * \brief Type definitions (as a part of public API).
 *
 * \copyright
 * SPDX-FileCopyrightText: 2021 Junde Yhi <junde@yhi.moe>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stddef.h>

/**
 * \brief Class for configuration for this project.
 */
typedef struct {
  /**
   * \brief Handler for reading a single line of HTTP text.
   *
   * This platform-specific callback needs to be implemented by the Thing
   * implementor for this project to read the incoming HTTP request. As the name
   * of this function suggests, each call should store a line of text, broken
   * by the Line Feed (LF, `\n`) character, with the LF included, plus a NUL
   * (`\0`), into `linebuf`. For example, for the following stream:
   *
   * ```
   * GET /example HTTP/1.1\r\nHost: thing.example.com\r\n\r\n
   * ```
   *
   * A call of this function should store the following into `linebuf`:
   *
   * ```
   * GET /example HTTP/1.1\r\n\0
   * ```
   *
   * Expected return values from this project are documented below.
   *
   * \param[inout] linebuf TinyWoTHTTPSimpleConfig::linebuf.
   * \param[in] bufsize TinyWoTHTTPSimpleConfig::linebuf_size.
   * \param[inout] ctx TinyWoTHTTPSimpleConfig::ctx.
   * \return
   * - 1 on a successful read of a line.
   * - 0 on a successful read, but a line feed is not found.
   * - -1 on end-of-stream (EOS); a failed read.
   * - -2 on any other failure.
   */
  int (*readln)(char *linebuf, size_t bufsize, void *ctx);
  /**
   * \brief Handler for writing a single line of HTTP text.
   *
   * This platform-specific callback needs to be implemented by the Thing
   * implementor for this project to write the outgoing HTTP response. As the
   * name of this function suggests, each call should send the line of text,
   * broken by the Line Feed (LF, `\n`) character, with the LF included, to e.g.
   * a network socket. As guarantees provided by this project, the implementor
   * may assume the following:
   *
   * - `linebuf` MUST NOT be NULL. However, an empty string will be passed to
   * send the HTTP end-of-header-section sequence (a CRLF).
   * - `linebuf` MUST be a HTTP line as well as a C string (ending with CR LF
   * NUL (`\r\n\0`)).
   *
   * Expected return values from this project are documented below.
   *
   * \param[inout] linebuf TinyWoTHTTPSimpleConfig::linebuf, but with NUL
   * guaranteed.
   * \param[inout] ctx TinyWoTHTTPSimpleConfig::ctx.
   * \return
   * - 1 on a successful write of a line.
   * - 0 on a successful write, but a line feed is not found.
   * - -1 on any other failure.
   */
  int (*writeln)(char *linebuf, void *ctx);
  /**
   * \brief Buffer holding lines read with #readln.
   *
   * This acts as a "scratchpad" for this project, so this project does not
   * allocate more memory via `malloc`. This is also essentially what is passed
   * to the `linebuf` parameter of #readln and #writeln.
   *
   * Note that the size of this buffer (#linebuf_size) limits the maximum size
   * of incoming and outcoming HTTP requests and responses (because of how
   * #readln and #writeln work).
   */
  char *linebuf;
  /**
   * \brief Size of #linebuf in bytes.
   *
   * This is essentially what is passed to the `bufsize` parameter of #readln
   * and #writeln.
   */
  size_t linebuf_size;
  /**
   * \brief Buffer holding the HTTP resource path copied out from the request.
   *
   * Note that the size of this buffer (#pathbuf_size) limits the maximum size
   * of the incoming HTTP path (it's also constrained by #linebuf_size, which
   * limits the maximum line length).
   */
  char *pathbuf;
  /**
   * \brief Size of #pathbuf in bytes.
   */
  size_t pathbuf_size;
  /**
   * \brief An arbitrary context (user data) to carry.
   *
   * For example, a network socket object can be carried here.
   */
  void *ctx;
} TinyWoTHTTPSimpleConfig;
