/**
 * \file api.h
 * \brief Public API definitions.
 *
 * \copyright
 * SPDX-FileCopyrightText: 2021 Junde Yhi <junde@yhi.moe>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <tinywot.h>

#include "type.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Receive and parse an incoming HTTP request.
 *
 * \param[inout] config A configuration object for this function to work.
 * \param[out] request A TinyWoT Web Thing request.
 */
int tinywot_http_simple_recv(TinyWoTHTTPSimpleConfig *config,
                             TinyWoTRequest *request);

/**
 * \brief Synthesize and send an outcoming HTTP response.
 *
 * \param[inout] config A configuration object for this function to work.
 * \param[in] response A TinyWoT Web Thing response.
 */
int tinywot_http_simple_send(TinyWoTHTTPSimpleConfig *config,
                             TinyWoTResponse *response);

#ifdef __cplusplus
}
#endif
