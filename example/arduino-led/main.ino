/*
 * An example Web Thing exposing a LED via HTTP on Arduino using TinyWoT,
 * TinyWoT-HTTP-Simple, and Ethernet.
 *
 * The following resources are available in this example:
 *
 * - `/led`: the Arduino built-in LED; property; read-write.
 * - `/toggle`: flip the status of LED; action.
 *
 * To read a property, send a GET. To write a property, send a PUT. To invoke
 * an action, send a POST. For example, assuming the LED is off, to turn it on,
 * either send:
 *
 * ```
 * PUT /led HTTP/1.1
 * Content-Type: text/plain
 * Content-Length: 4
 *
 * true
 * ```
 *
 * ... or send:
 *
 * ```
 * POST /toggle HTTP/1.1
 *
 * ```
 *
 * SPDX-FileCopyrightText: 2021 Junde Yhi <junde@yhi.moe>
 * SPDX-License-Identifier: MIT
 */

#include <Ethernet.h>
#include <tinywot-http-simple.h>

#define BAUD 9600
#define LED LED_BUILTIN

// Socket configurations
// https://www.arduino.cc/en/Reference/Ethernet
EthernetServer server = EthernetServer(80);
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip4(192, 168, 1, 11);

// Strings stored in the program space, avaiable on AVR platforms.
// Note that since reading them requires a special instruction, care must be
// taken when utilizing this feature. To tell TinyWoT and TinyWoT-HTTP-Simple
// about this, define `TINYWOT_USE_PROGMEM` and
// `TINYWOT_HTTP_SIMPLE_USE_PROGMEM`. Otherwise, the Thing will malfunction.
const char str_true[] PROGMEM = "true";
const char str_false[] PROGMEM = "false";
const char str_led[] PROGMEM = "/led";
const char str_toggle[] PROGMEM = "/toggle";
const char str_well_known_td[] PROGMEM = "/.well-known/wot-thing-description";
const char str_td[] PROGMEM =
  "{\"@context\":[\"https://www.w3.org/2019/wot/td/"
  "v1\"],\"@type\":[\"Thing\"],\"id\":\"urn:uuid:135a9cd2-aa55-4268-b1d1-"
  "e5b1a4827bb7\",\"title\":\"TinyWoT Arduino LED "
  "Example\",\"base\":\"http://"
  "192.168.1.11\",\"securityDefinitions\":{\"nosec_sc\":{\"scheme\":\"nosec\"}}"
  ",\"security\":[\"nosec_sc\"],\"properties\":{\"led\":{\"type\":\"boolean\","
  "\"title\":\"LED Status\",\"description\":\"Status of the Arduino built-in "
  "LED.\",\"forms\":[{\"href\":\"/"
  "led\"}]}},\"actions\":{\"toggle\":{\"title\":\"Toggle "
  "LED\",\"description\":\"Flip the status of the Arduino built-in "
  "LED.\",\"input\":{\"type\":\"boolean\"},\"output\":{\"type\":\"boolean\"},"
  "\"forms\":[{\"href\":\"/toggle\"}]}},\"events\":{}}";

// Forward declarations of thing implementation functions
// Function implementations are below loop()
int readln(char *linebuf, size_t bufsize, void *ctx);
int write(const char *buf, size_t nbytes, void *ctx);
TinyWoTResponse handler_led(TinyWoTRequest *req, void *ctx);
TinyWoTResponse handler_toggle(TinyWoTRequest *req, void *ctx);
TinyWoTResponse handler_td(TinyWoTRequest *req, void *ctx);

// Handlers implementing the behaviors of this Thing.
TinyWoTHandler handlers[] = {
  {str_led,
   WOT_OPERATION_TYPE_READ_PROPERTY | WOT_OPERATION_TYPE_WRITE_PROPERTY,
   handler_led, NULL},
  {str_toggle, WOT_OPERATION_TYPE_INVOKE_ACTION, handler_toggle, NULL},
  {str_well_known_td, WOT_OPERATION_TYPE_READ_PROPERTY, handler_td, NULL},
};

// The Thing.
TinyWoTThing thing = {
  .handlers = handlers,
  .handlers_size = sizeof(handlers) / sizeof(TinyWoTHandler),
};

void setup(void) {
  Serial.begin(BAUD);
  while (!Serial) {}

  Serial.println(F("Sample Web Thing based on TinyWoT with HTTP."));

  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  // Additional mitigation on an unused SD card
  // https://www.arduino.cc/en/Tutorial/LibraryExamples/WebServer/
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);

  // I use a MKR ETH Shield, which does not use a default CS pin.
  // Ethernet.init(5);

  Ethernet.begin(mac, ip4);
  server.begin();

  Serial.println(F("Done initialization."));
}

void loop(void) {
  static char linebuf[128];
  static char pathbuf[64];
  static TinyWoTRequest req;
  static TinyWoTResponse resp;
  static int r = 0;

  EthernetClient client = server.available();
  if (!client)
    return;

  memset(linebuf, 0, 128);
  memset(pathbuf, 0, 32);

  Serial.print(F("> "));
  Serial.print(client.remoteIP());
  Serial.print(F(":"));
  Serial.print(client.remotePort());
  Serial.println();

  static TinyWoTHTTPSimpleConfig cfg = {
    .readln = readln,
    .write = write,
    .linebuf = linebuf,
    .linebuf_size = 128,
    .pathbuf = pathbuf,
    .pathbuf_size = 64,
    .ctx = &client,
  };

  r = tinywot_http_simple_recv(&cfg, &req);
  if (!r) {
    Serial.println(F("! Error on receiving HTTP request; closing connection."));
    client.stop();
    return;
  }

  resp = tinywot_process(&thing, &req);

  r = tinywot_http_simple_send(&cfg, &resp);
  if (!r) {
    Serial.println(F("! Error on sending HTTP response; closing connection."));
    client.stop();
    return;
  }

  Serial.println(F("< Responded; closing connection."));

  client.stop();
}

// Read and write handlers, required by TinyWoT-HTTP-Simple. In this example,
// they read from and write to an EthernetClient.

int readln(char *linebuf, size_t bufsize, void *ctx) {
  EthernetClient *client = (EthernetClient *)ctx;

  if (client->peek() == -1)
    return -1; // EOS before reading anything

  for (char *ptr = linebuf; bufsize; ptr++, bufsize--) {
    if (client->peek() == -1) {
      *ptr = '\0';
      return 0; // EOS, but have read something
    }

    *ptr = client->read();

    if (*ptr == '\n')
      return 1;
  }

  return 0; // Buffer ends before encountering LF
}

int write(const char *buf, size_t nbytes, void *ctx) {
  EthernetClient *client = (EthernetClient *)ctx;
  client->write(buf, nbytes);
  return 1;
}

// Handlers implementing the behaviors of this Thing.

TinyWoTResponse handler_led(TinyWoTRequest *req, void *ctx) {
  (void)ctx;
  TinyWoTResponse resp;

  if (req->op == WOT_OPERATION_TYPE_READ_PROPERTY) {
    int led = digitalRead(LED);
    resp.status = TINYWOT_RESPONSE_STATUS_OK;
    resp.content_type = TINYWOT_CONTENT_TYPE_JSON;
    if (led) {
      resp.content_length = strlen_P(str_true);
      resp.content = (void *)str_true;
    } else {
      resp.content_length = strlen_P(str_false);
      resp.content = (void *)str_false;
    }
  } else if (req->op == WOT_OPERATION_TYPE_WRITE_PROPERTY) {
    if (strcmp_P((char *)req->content, str_true) == 0) {
      digitalWrite(LED, HIGH);
      resp.status = TINYWOT_RESPONSE_STATUS_OK;
      resp.content_type = TINYWOT_CONTENT_TYPE_JSON;
      resp.content_length = strlen_P(str_true);
      resp.content = (void *)str_true;
    } else if (strcmp_P((char *)req->content, str_false) == 0) {
      digitalWrite(LED, LOW);
      resp.status = TINYWOT_RESPONSE_STATUS_OK;
      resp.content_type = TINYWOT_CONTENT_TYPE_JSON;
      resp.content_length = strlen_P(str_false);
      resp.content = (void *)str_false;
    } else {
      resp.status = TINYWOT_RESPONSE_STATUS_BAD_REQUEST;
    }
  } else {
    resp.status = TINYWOT_RESPONSE_STATUS_UNSUPPORTED;
  }

  return resp;
}

TinyWoTResponse handler_toggle(TinyWoTRequest *req, void *ctx) {
  (void)ctx;
  TinyWoTResponse resp;

  resp.status = TINYWOT_RESPONSE_STATUS_OK;
  resp.content_type = TINYWOT_CONTENT_TYPE_JSON;

  if (digitalRead(LED)) {
    digitalWrite(LED, LOW);
    resp.content_length = strlen_P(str_false);
    resp.content = (void *)str_false;
  } else {
    digitalWrite(LED, HIGH);
    resp.content_length = strlen_P(str_true);
    resp.content = (void *)str_true;
  }

  return resp;
}

TinyWoTResponse handler_td(TinyWoTRequest *req, void *ctx) {
  (void)ctx;
  TinyWoTResponse resp;

  resp.status = TINYWOT_RESPONSE_STATUS_OK;
  resp.content_type = TINYWOT_CONTENT_TYPE_JSON;
  resp.content_length = strlen_P(str_td);
  resp.content = (void *)str_td;

  return resp;
}
