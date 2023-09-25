// This is the dashboard node. The web interface for the IoT network is hosted here
// This webserver node has a static IP address of 192.168.0.101

// for RFID:
// UID tag : BA E6 43 73

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebSrv.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

// for RFID reader
#include <SPI.h>
#include <MFRC522.h>

// pins for RFID
#define RST_PIN 0 // D3 = GPIO 0
#define SS_PIN  2 // D4 == GPIO 2

// instantiate RFID reader
MFRC522 rfid(SS_PIN, RST_PIN);

// async webserver
AsyncWebServer server(80);


// set up a static IP address
IPAddress staticIP(192, 168, 0, 101); // Sensor Node IP
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 0, 1);

// list IP addresses of all nodes in the network
IPAddress ultrasonicIP(192, 168, 0, 102);
IPAddress servoIP();
//IPAddress cam1IP(192, 168, 0, 104);
//IPAddress cam2IP(192, 168, 0, 105);
//IPAddress climate1(192, 168, 0, 37);
//IPAddress climate1(192, 168, 0, 113);

// office credentials
  const char* ssid =  "";
  const char* password = "";

void get_index(AsyncWebServerRequest *request) {
    String html = R"EOF(
<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESPHomeSecurity Dashboard</title>
    <style>
        body {
          font-family: Arial, sans-serif;
          background-color: #f4f4f4;
          text-align: center;
          padding: 50px;
          margin: 0;
        }
        
        .container {
          max-width: 1200px;
          margin: 0 auto;
        }
        
        button {
          padding: 10px 20px;
          border: none;
          border-radius: 5px;
          margin: 10px;
          cursor: pointer;
        }
        
        .on {
          background-color: #4CAF50;
          color: #fff;
        }
        
        .off {
          background-color: #f44336;
          color: #fff;
        }
        
        .card {
          background-color: #ffffff;
          border: 1px solid #e0e0e0;
          border-radius: 5px;
          margin: 10px 5px;
          padding: 20px;
          box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
          flex: 1 0 calc(50% - 10px);
        }
        
        .card-title {
          font-size: 24px;
          font-weight: bold;
          margin-bottom: 15px;
        }
        
        .card-content {
          margin: 10px 0;
        }
        
        .card-group {
          display: flex;
          justify-content: space-between;
          flex-wrap: wrap;
          flex-basis: 50%;
        }
        
        /* on smaller screens, make each card full width */
        @media (max-width: 767px) {
          .card {
            min-width: 100%;
          }
        }
    </style>
</head>

<body>
    <div class="container">
    <h1>The ESPHomeSecurity Dashboard</h1>
    <p>Decentralised Privacy and Security</p>
    
      <div class="card-group">
          <!-- Climate Control Card -->
          <div class="card">
              <div class="card-title">Climate Control</div>
              <div class="card-content">
                  <p>Temperature: <span id="temperature-value">--</span>°C</p>
                  <p>Humidity: <span id="humidity-value">--</span>%</p>
              </div>
          </div>
          
          <!-- Door Alarm Card -->
          <div class="card">
              <div class="card-title">Door Alarm</div>
              <div class="card-content">
                  <button class="off" onclick="setAlarmStatus(0)">Turn Off</button>
                  <button class="on" onclick="setAlarmStatus(1)">Turn On</button>
              </div>
          </div>
          
          <!-- Cam 1 Card -->
          <div class="card">
              <div class="card-title">Cam 1</div>
              <div class="card-content" id="streamContainer1">
                  <!-- Stream for Cam 1 will be appended here -->
              </div>
          </div>
  
          <!-- Cam 2 Card -->
          <div class="card">
              <div class="card-title">Cam 2</div>
              <div class="card-content" id="streamContainer2">
                  <!-- Stream for Cam 2 will be appended here -->
              </div>
          </div>
      </div>
  </div>

    <script>
        function setAlarmStatus(status) {
          var xhr = new XMLHttpRequest();
          xhr.open('GET', 'http://192.168.0.108/setAlarmStatus?s=' + (status == 1 ? '1' : '0'), true);
          xhr.send();
        }

        window.onload = function() {
          loadStream('http://192.168.0.104:81/stream', 'streamContainer1');  // cam 1
//          loadStream('http://192.168.0.106:81/stream', 'streamContainer2');  // cam 2 (before module failure)

          // fetch the readings once when the page loads
          fetchTemperatureAndHumidity();

          // fetch the readings every 10 seconds
          setInterval(fetchTemperatureAndHumidity, 10000);
      };
      
      function loadStream(url, containerId) {
          var img = new Image();
          img.src = url;
      
          img.onload = function() {
              // stream is available
              var streamContainer = document.getElementById(containerId);
              var streamImg = document.createElement('img');
              streamImg.src = img.src;
              streamImg.width = 640;
              streamImg.height = 480;
              streamContainer.appendChild(streamImg);
          };
      
          img.onerror = function() {
              // stream is not available, handle error
              console.error("The stream from " + url + " is not available.");
          };
      }
      function fetchTemperatureAndHumidity() {
          var xhr = new XMLHttpRequest();
          xhr.open('GET', 'http://192.168.0.113/climate', true);
          xhr.onload = function() {
              if (this.status == 200) {
                  var data = JSON.parse(this.responseText);
                  document.getElementById("temperature-value").innerText = data.temperature.toFixed(2);
                  document.getElementById("humidity-value").innerText = data.humidity.toFixed(2);
              }
          };
          xhr.send();
      }

    </script>
</body>

</html>
)EOF";

    request->send(200, "text/html", html);
}

/////////////// RFID functions ////////////////////
void checkRFIDAndDisableAlarm() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
    return;

  // convert UID to a string and compare with the known acceptable UID of the module.
  String readUID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    readUID += String(rfid.uid.uidByte[i], HEX);
  }

  String authorizedUID = "bae64373";  // UID tag in lowercase
  Serial.println(authorizedUID);
  Serial.println(readUID);
  delay(2000);
  if (readUID == authorizedUID) {
    // send request to turn off the alarm
    turnOffAlarm();
    Serial.println(readUID);
  }
}

// send GET request to alarm node to turn off alarm
void turnOffAlarm() {
  WiFiClient client; // create WiFiClient object
  HTTPClient http;
  
  if (http.begin(client, "http://192.168.0.108/setAlarmStatus?s=0")) {
    int httpCode = http.GET(); // Send the request

    if(httpCode > 0) { // Check the returning code
        String payload = http.getString();  // get request response payload
        Serial.println(payload);            // print response payload
    }
    
    // close connection
    http.end();
  } else {
    Serial.println("HTTP client begin failed.");
  }
}
/////////////// RFID functions end ////////////////////

void setup() {
  // initialize the serial monitor
  Serial.begin(115200);

  // configure the static IP address
  WiFi.config(staticIP, gateway, subnet, dns);

  
  // connect to WiFi network
  WiFi.begin(ssid, password);
  
  // ensure wifi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Waiting to connect...");
  }

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", get_index); // get the index page on route router

  server.begin(); 
  Serial.println("Server listening");

  // RFID sensor
  SPI.begin(); 
  rfid.PCD_Init();
  }

void loop() {
  // check for RFID readings
  checkRFIDAndDisableAlarm();
  delay(30); 
}
