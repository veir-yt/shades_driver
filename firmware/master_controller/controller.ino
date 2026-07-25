#include <WiFi.h>
#include <WebServer.h>
#include <uri/UriBraces.h>
#include "shared.h"
#include "env.h"

// wifi setup
const char* ssid = WIFI_SSID; // replace with your credentials...
const char* password = WIFI_PW; // mine I linked through the .env file

#define LED_BUILTIN 2

WebServer server(80);

void get_page() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
      <body>
        <div>
          <h1>
            Hello World
          </h1>
          <h3>
            This is <strong>VEIR</strong>
          </h3>
        </div>
      </body>
      <script>
        console.log('yoooo');

        async function get_data(){
          try{
            const r = await fetch('/users/veir', { cache: 'no-store' });
            const text = await r.text();
            console.log(text);
          } catch(e){
            console.log(e);
          }
        }
        get_data();
      </script>
    </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(SERIAL_SPEED);

  WiFi.begin(ssid, password);
  Serial.println("Connecting");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting...");
  }

  delay(500);

  digitalWrite(LED_BUILTIN, HIGH);

  server.on("/", get_page);

  server.on(UriBraces("/users/{}"), []() {
    String user = server.pathArg(0);
    server.send(200, "text/plain", "User: '" + user + "'");
  });

  server.begin();

  Serial.println(WiFi.localIP());
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH);
    server.handleClient();
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
}
