# Arduino LED

An example Web Thing exposing a LED via HTTP on Arduino using [TinyWoT], [TinyWoT-HTTP-Simple], and [Ethernet].

To run the sketch, use:

- Arduino Uno, or any higher-end boards
- Any Arduino Ethernet library supported Ethernet connectivity (e.g. shields)

The following resources are available in this example:

- `/led`: the Arduino built-in LED; property; read-write.
- `/toggle`: flip the status of LED; action.

To read a property, send a `GET`. To write a property, send a `PUT`. To invoke an action, send a `POST`. For example, assuming the LED is off, to turn it on, either send:

```
PUT /led HTTP/1.1
Content-Type: application/json
Content-Length: 4

true
```

... or send:

```
POST /toggle HTTP/1.1

```

[arduino-led.td.json](arduino-led.td.json) is the [Thing Description](https://www.w3.org/TR/wot-thing-description11/) describing this Web Thing implemented in [main.ino](main.ino). The Thing Description can also be fetched at `/.well-known/wot-thing-description`, making it [discoverable](https://www.w3.org/TR/wot-discovery/#introduction-well-known) via well-known URI.

The IP addresss is hardcoded to `192.168.1.11` in both the implementation and the Thing Description; change it on demand.

[TinyWoT]: https://github.com/lmy441900/tinywot
[TinyWoT-HTTP-Simple]: https://github.com/lmy441900/tinywot-http-simple
[Ethernet]: https://www.arduino.cc/en/Reference/Ethernet
