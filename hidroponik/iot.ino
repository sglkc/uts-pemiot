#include <WiFi.h>
#include "PubSubClient.h"
#include "DHTesp.h"

const char* ssid = "Wokwi-GUEST"; // Add your WiFi SSID
const char* password = ""; // Add your WiFi Password

const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;

#define DHT_PIN 32;
#define RELAY_PIN 13;
#define LED_RED 18
#define LED_YELLOW 5
#define LED_GREEN 17
#define BUZZER_PIN 8

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
DHTesp dhtSensor;

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Callback - Message: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Hello, ESP32!");
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Connected to Wi-Fi");

  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(callback);
}

void loop() {
  if (!mqttClient.connected()) {
    Serial.println("Connecting to MQTT Broker...");

    while (!mqttClient.connected()) {
      Serial.println("Reconnecting to MQTT Broker..");
      String clientId = "ESP32Client-";
      clientId += String(random(0xffff), HEX);

      if (mqttClient.connect(clientId.c_str())) {
        Serial.println("Connected.");
      }
    }
  }

  mqttClient.loop();

  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  float temperature = data.temperature;
  float humidity = data.humidity;

  bool isRedActive = false;
  bool isYellowActive = false;
  bool isGreenActive = false;
  bool isBUZZER_PINActive = false;

  if (temperature > 35) {
    digitalWrite(LED_RED, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    isRedActive = true;
    isBUZZER_PINActive = true;
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_GREEN, LOW);
  } else if (temperature >= 30 && temperature <= 35) {
    digitalWrite(LED_YELLOW, HIGH);
    isYellowActive = true;
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  } else {
    digitalWrite(LED_GREEN, HIGH);
    isGreenActive = true;
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }

  if (temperature >= 35) {
    digitalWrite(RELAY_PIN, HIGH);
  } else {
    digitalWrite(RELAY_PIN, LOW);
  }

  char tempString[8];
  char humString[8];

  dtostrf(temperature, 1, 2, tempString);
  dtostrf(humidity, 1, 2, humString);

  mqttClient.publish("sensor/temperature", tempString);
  mqttClient.publish("sensor/humidity", humString);

  String statusMessage = String("Red LED: ") + (isRedActive ? "ON" : "OFF") +
                         ", Yellow LED: " + (isYellowActive ? "ON" : "OFF") +
                         ", Green LED: " + (isGreenActive ? "ON" : "OFF") +
                         ", BUZZER_PIN: " + (isBUZZER_PINActive ? "ON" : "OFF") +
                         ", RELAY_PIN: " + ((temperature >= 35) ? "ON" : "OFF");

  mqttClient.publish("sensor/status", statusMessage.c_str());

  delay(2000);
}
