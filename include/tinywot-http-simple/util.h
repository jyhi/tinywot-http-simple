/**
 * \file util.h
 * \brief Utility function definitions.
 *
 * \copyright
 * SPDX-FileCopyrightText: 2021 Junde Yhi <junde@yhi.moe>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Test up to `count` bytes if two strings are case-insensitively equal.
 *
 * \param[in] s1 A string.
 * \param[in] s2 Another string.
 * \param[in] count Up to how many bytes to compare.
 * \return non-zero if `s1[:count] == s2[:count]`, otherwise 0.
 */
int strinequ(const char *s1, const char *s2, size_t count);

#ifdef __cplusplus
}
#endif
