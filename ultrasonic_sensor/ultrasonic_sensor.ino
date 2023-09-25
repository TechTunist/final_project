// This sketch is for a door alarm, implemented with an ultrasonic sensor
// The esp8266 has a static IP address of 192.168.0.102
// If the sensor measures a distance below the set minimum an alarm triggers
// This sensor is controllable via the web dashboard but has its own web page as redundancy
// This sensor also operates as a standalone alarm if there is no local network connection

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// set port of web server
ESP8266WebServer server(80);

// enter WiFi credentials here
  const char* ssid =  "";
  const char* password = "";

// set up a static IP address
IPAddress staticIP(192, 168, 0, 102); 
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 0, 1);

// trigger, echo, and buzzer pins
const int trigPin = 2; // GPIO 2
const int echoPin = 15; // GPIO 15
const int buzzerPin = 16; // GPIO 4 for buzzer

// duration and distance variables
long duration = 0;
int distance = 20; // initialise to 20 as it is above threshold value for buzzer

// smooth out the readings to avoid false positives
const int numReadings = 3; 
int readings[numReadings];   
int readIndex = 0;          
int total = 0;               
int averageDistance = 20;    

// bool to control buzzer
bool alarmActive = false;

// set pins for led indication of operation mode (standalone or wifi)
const int standaloneLedPin = 4; // GPIO 5 corresponds to D1
const int wifiLedPin = 5;       // GPIO 4 corresponds to D2

// create backup dashboard for remote control of alarm node
void get_index() {
  String html = "<!DOCTYPE html> <html> ";
  html += "<head><meta http-equiv=\"refresh\" content=\"2\"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"></head>";
  html += "<body> <h1>The ESPHomeSecurity Dashboard</h1>";
  html += "<p> Ultrasonic Sensor Node </p>";
  html += "<div> <p> <strong> The Distance to ultrasonic sensor is: ";
  html += distance;
  html += " cm</strong> </p> </div>";
  html += "<div> <p> <strong> The time taken for the signal to travel is: ";
  html += duration;
  html += " ms</strong> </p> </div>";
  html += "<button onclick=\"setAlarmStatus(0)\">Turn Off</button>";
  html += "<button onclick=\"setAlarmStatus(1)\">Turn On</button>";
  html += "</body> </html>";

  // javaScript function for AJAX handling of GET requests from dashboard to set alarm status
  html += "<script>";
  html += "function setAlarmStatus(status) {";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('GET', '/setAlarmStatus?s=' + status, true);";
  html += "  xhr.send();";
  html += "}";
  html += "</script>";
  html += "</body> </html>";

  server.send(200, "text.html", html);
}

void setup() {
  // initialize the serial monitor
  Serial.begin(115200);

  // initialise led pins
  pinMode(standaloneLedPin, OUTPUT);
  pinMode(wifiLedPin, OUTPUT);

  // setup static IP address
  WiFi.config(staticIP, gateway, subnet, dns);
  
  // connect to WiFi network
  WiFi.begin(ssid, password);

  // set piinmodes for ultrasonice sensor
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzerPin, OUTPUT);

  // try to connect to WiFi for a limited number of attempts t oprevent blocking and allow standalone mode
  int attempts = 0;
  const int maxAttempts = 15;
  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    delay(500);
    Serial.println("Waiting to connect...");
    attempts++;
  }
  
  if(WiFi.status() == WL_CONNECTED) {
    
    // wifi mode initiated
    digitalWrite(standaloneLedPin, LOW);
    digitalWrite(wifiLedPin, HIGH);

    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("Connected to WiFi. Running in WiFi mode.");

    server.on("/", get_index); // get the index page on route router

    server.on("/setAlarmStatus", []() {
      String s = server.arg("s");
      if (s == "1") {
        alarmActive = true;
      } else if (s == "0") {
        alarmActive = false;
        digitalWrite(buzzerPin, LOW);
      }

      // allow cross origin requests from dashboard IP only for security
      server.sendHeader("Access-Control-Allow-Origin", "http://192.168.0.101");
      server.send(204); // No content response
    });

    server.begin();
    Serial.println("Server listening");
  } else {
    Serial.println("Failed to connect to WiFi. Running standalone.");
    digitalWrite(standaloneLedPin, HIGH);
    digitalWrite(wifiLedPin, LOW);
  }

  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }
}

void loop() {
  // handling of incoming client requests
  server.handleClient();

  // measures and prints the distance
  distanceCentimeter();

  // check if average distance below threshold and activate buzzer
  if (averageDistance < 10) {
    alarmActive = true;
  }

  // if alarmActive == true , keep the buzzer ON
  if (alarmActive) {
    digitalWrite(buzzerPin, HIGH);
  } else {
    digitalWrite(buzzerPin, LOW);
  }

  delay(30); 
}

void distanceCentimeter(){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = (duration * 0.034) / 2;

  // subtract the last reading
  total = total - readings[readIndex];
  
  // read sensor
  readings[readIndex] = distance;
  
  // add reading to total
  total = total + readings[readIndex];
 
  readIndex = readIndex + 1;

  // go back to position 0 of the array when reaching the end
  if (readIndex >= numReadings) {
    readIndex = 0;
  }

  // calculate average distance
  averageDistance = total / numReadings;

  Serial.print(averageDistance);
  Serial.println(": Average Centimeters");
}
