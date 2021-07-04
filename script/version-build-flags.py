#!/usr/bin/env python3
#
# Script to extract version information from library.json and print preprocessor
# flags for PlatformIO to inject them into the code.
#
# SPDX-FileCopyrightText: 2021 Junde Yhi <junde@yhi.moe>
# SPDX-License-Identifier: MIT

import json

with open("library.json", "r") as library_json:
  library = json.load(library_json)

http_simple_ver = library["version"]
for dep in library["dependencies"]:
  if dep["name"] == "tinywot":
    tinywot_ver = dep["version"]
  else:
    tinywot_ver = "0" # XXX

print("-D TINYWOT_HTTP_SIMPLE_VERSION='\"{}\"' -D TINYWOT_VERSION='\"{}\"'"
  .format(http_simple_ver, tinywot_ver))
