#!/bin/sh
#
# Convenient script for invoking PlatformIO automatic (CI) builds.
#
# SPDX-FileCopyrightText: 2021 Junde Yhi <junde@yhi.moe>
# SPDX-License-Identifier: MIT

set -e

platformio ci \
  --lib . \
  --lib ../tinywot \
  --project-option 'lib_deps=arduino-libraries/Ethernet' \
  --board uno \
  example/arduino-led/main.ino

platformio ci \
  --lib . \
  --lib ../tinywot \
  --project-option 'lib_deps=arduino-libraries/Ethernet' \
  --project-option 'build_flags=-D TINYWOT_USE_PROGMEM -D TINYWOT_HTTP_SIMPLE_USE_PROGMEM' \
  --board uno \
  example/arduino-led/main.ino
