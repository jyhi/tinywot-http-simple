<!--
SPDX-FileCopyrightText: 2021 Junde Yhi <junde@yhi.moe>
SPDX-License-Identifier: CC0-1.0
-->

# TinyWoT Protocol Binding - Simple HTTP

This is a [protocol binding][pb] implementation for [TinyWoT] to transform HTTP requests into [TinyWoT] requests and [TinyWoT] responses into HTTP responses.

[pb]: https://www.w3.org/TR/wot-binding-templates/
[TinyWoT]: https://github.com/lmy441900/tinywot

## Installation

- [PlatformIO]: `pio lib install tinywot-http-simple`

[PlatformIO]: https://platformio.org/

## Use

1. Prepare a configuration object (`TinyWoTHTTPSimpleConfig`). This include:
  - a read line handler (`readln`)
  - a write handler (`write`)
  - a buffer "scratchpad" (`linebuf`) and its size (`linebuf_size`)
  - a buffer storing the path (`pathbuf`) and its size (`pathbuf_size`)
  - an optional context pointer (`ctx`) for the use of read / write handlers; for example, a socket
2. Upon a network request, invoke `tinywot_http_simple_recv` with the configuration object and a pointer to `TinyWoTRequest`. The function will fill the `TinyWoTRequest` while consuming the HTTP request.
3. After `tinywot_process`, invoke `tinywot_http_simple_send` with the configuration object and a pointer to the `TinyWoTResponse` returned. The function will emit HTTP response texts according to the `TinyWoTResponse`.

```c
if (!tinywot_http_simple_recv(&cfg, &req)) {
  // error handling
}

resp = tinywot_process(&thing, &req);

if (!tinywot_http_simple_send(&cfg, &resp)) {
  // error handling
}
```

A sample Thing implemented using this library based on Arduino with Ethernet connectivity can be found in [example/arduino-led](example/arduino-led).

## Limitations

- This library parses HTTP line-by-line, so the size of buffer (`linebuf`, as the name suggests) essentially limits the maximum possible length of a HTTP request. If a HTTP request has a line longer than the size of buffer, the receiving or the sending process will fail. It's recommended to set `linebuf_size` to a value larger than 48 (bytes).

## License

This project is [REUSE 3.0][reuse] compliant: every file is carried with its own copyright and licensing information in the comment headers or the corresponding `.license` files. In general:

- Source files are licensed under the MIT license.
- Various data files are (un)licensed under the CC0 license.

See the [LICENSES](LICENSES) directory for copies of licenses used across this project. The [LICENSE](LICENSE) file also contains a MIT license for non-REUSE practices.

[reuse]: https://reuse.software/spec/
