// This is the sketch that reads the potentiometer value
// and sends it via espnow to the servo node to rotate the camera

// potentiometer node MAC: AC:0B:FB:D7:53:E8

#include <ESP8266WiFi.h>
#include <espnow.h>

#define POTENTIOMETER_PIN A0

// define MAC address of the receiver servo node
uint8_t servoMACAddress[] = {0x40, 0x91, 0x51, 0x4F, 0x0A, 0x56};

// create data structure for transfer to of potentiometer value to servo node
typedef struct struct_message {
    int servoPosition;
} struct_message;

struct_message data_struct;

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    if (esp_now_init() != 0) {
        Serial.println("ESP-NOW Init Failed");
        return;
    }
    // configure esp now connection
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    esp_now_add_peer(servoMACAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
}

void loop() {
  // get value from potentiometer and map to angles for servo
    int potValue = analogRead(POTENTIOMETER_PIN);
    data_struct.servoPosition = map(potValue, 0, 1023, 0, 180);

    // send data over esp now
    esp_now_send(servoMACAddress, (uint8_t *)&data_struct, sizeof(data_struct));
    Serial.println(data_struct.servoPosition);
    delay(50);
}
