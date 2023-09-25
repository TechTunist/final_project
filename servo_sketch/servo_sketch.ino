// This sketch is for the esp8266 servo node that receives
// the rotational information over espnow to direct the camera

// Servo node MAC: 40:91:51:4F:0A:56

#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Servo.h>

#define SERVO_PIN 5 // D1

// initialise servo object
Servo servo;

// create struct to receive data from potentiometer node
typedef struct struct_message {
    int servoPosition;
} struct_message;

struct_message incomingData;

void setup() {
    Serial.begin(115200);
    servo.attach(SERVO_PIN);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    if (esp_now_init() != 0) {
        Serial.println("ESP-NOW Init Failed");
        return;
    }
    // configure esp now connection
    esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
    esp_now_register_recv_cb(receiveCallBack);
}

void loop() {
    // loop empty as callback function deals with functionality
}

// define callback function to handle when data arrives over esp now
void receiveCallBack(uint8_t *senderMAC, uint8_t *data, uint8_t len) {
    memcpy(&incomingData, data, sizeof(incomingData));
    servo.write(incomingData.servoPosition);
    Serial.println(incomingData.servoPosition);
}
