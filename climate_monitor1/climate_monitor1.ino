// This is the code for flashing the climate sensor device that provides local temperature and humidity data

#include <ESP8266WiFi.h>
#include <DHT.h>

// add WiFi credentials here
  const char* ssid =  "";
  const char* password = "";

// DHT11 configuration
#define DHTPIN 2  // this is the GPIO pin connected to the DHT11 data pin
#define DHTTYPE DHT11

int sensePin = 2;
#define Type DHT11


DHT dht(sensePin, Type);
WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  delay(10);

  dht.begin();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    client.setTimeout(2000);  // set a 2-second timeout
    String request = client.readStringUntil('\r');
    
    if (request.indexOf("/climate") != -1) {
      float humidity = dht.readHumidity();
      float temperature = dht.readTemperature();

      delay(100);

      Serial.println(temperature);
      Serial.println(humidity);

      // prepare the response JSON
      String response = "{\"temperature\":";
      response += String(temperature);
      response += ",\"humidity\":";
      response += String(humidity);
      response += "}";

      Serial.println(response);

      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Access-Control-Allow-Origin: http://192.168.0.101"); // allow CORS requests from dashboard IP
      client.println("Content-Length: " + String(response.length()));
      // client.println("Connection: close");
      client.println();
      delay(100);
      client.println(response);
      delay(500);
    }
    client.stop();
  }

  delay(10000);
}
