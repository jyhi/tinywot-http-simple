/**
 * \file util.h
 * \brief Utility function implementations.
 *
 * \copyright
 * SPDX-FileCopyrightText: 2021 Junde Yhi <junde@yhi.moe>
 * SPDX-License-Identifier: MIT
 */

#include <ctype.h>
#include <stddef.h>

#include "util.h"

int strinequ(const char *s1, const char *s2, size_t count) {
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
