#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "time.h"

// NTP Server (to get the real-time clock)
const char* ntpServer2 = "time.google.com";
const char* ntpServer3 = "time.windows.com";

const long  gmtOffset_sec = 19800;  // GMT+5:30 for IST
const int   daylightOffset_sec = 0;

// WiFi Credentials
const char* ssid = "";//add your ssid here
const char* password = "";//add your password here

// MQTT Broker Details
const char* mqtt_server = "3.111.85.110"; // Replace with your AWS Elastic IP
const int mqtt_port = 1883;  // Use 8883 if using SSL
const char* mqtt_user = "your_mqtt_user"; // Optional, only if authentication is enabled
const char* mqtt_password = "your_mqtt_password"; 

// MQTT Topic
const char* topic = "CG001";

// Buzzer Pin
#define BUZZER_PIN 5  // Use the GPIO pin where the buzzer is connected

WiFiClient espClient;
PubSubClient client(espClient);
bool sentTimestamp = false; // Flag to ensure timestamp is sent only once
String timestamp;

void setup_wifi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived: ");
  String msg = "";
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.println(msg);

  // Control Buzzer
  if (msg == "ON") {
    digitalWrite(BUZZER_PIN, HIGH);  // Turn buzzer ON
    Serial.println("buzzer ON");
  } else if (msg == "OFF") {
    digitalWrite(BUZZER_PIN, LOW);   // Turn buzzer OFF
    Serial.println("buzzer OFF");
  }
}
String getTimestamp() {
    struct tm timeinfo;

    Serial.println("Trying to obtain time...");
    
    // Try different NTP servers
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer2);
    delay(2000); // Wait for time to sync
    
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed with time.google.com, trying time.windows.com...");
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer3);
        delay(2000);
    }
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time from all servers!");
        return "0";  // Return "0" if time is unavailable
    }

    Serial.println("Time successfully obtained!");
    char timeString[30];
    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(timeString);
}
void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_password)) {
      Serial.println("Connected!");
      client.subscribe(topic);
        if (!sentTimestamp) {
        timestamp = getTimestamp();
        Serial.print("Publishing timestamp: ");
        Serial.println(timestamp);
        client.publish(topic, timestamp.c_str());
        sentTimestamp = true; // Prevents multiple publishing
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
