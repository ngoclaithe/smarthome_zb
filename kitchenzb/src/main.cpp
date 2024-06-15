#include <Wire.h>
#include <HardwareSerial.h>
#include "OneButton.h"
#include <DHT.h>
#include "esp_task_wdt.h"
#include <BH1750.h>
#define DHTTYPE DHT11

String local_add = "0xbc";
int nutnhan1 = 14;
int nutnhan2 = 13;
const int lightPin = 26;
const int dhtPin = 19;    
const int buzzerPin = 5; 
const int mq2Pin = 34;
float temperature = 0.0;
float humidity = 0.0;
int buzzerState = 0;
int lightState = 0;
int mq2=0;
float lux=0.0;
OneButton button1(nutnhan1, true);
OneButton button2(nutnhan2, true);
HardwareSerial zigbeeSerial(2); 
DHT dht(dhtPin, DHTTYPE);
BH1750 lightMeter;
void changeBuzzerState();
void readSensor();
void sendStatus();
void receiveCommand();
void changeLightState();
void changeBuzzerState();
void setup() {

  Serial.begin(9600);
  zigbeeSerial.begin(9600, SERIAL_8N1, 17, 16); 
  Wire.begin();
  lightMeter.begin();
  pinMode(buzzerPin, OUTPUT);
  pinMode(lightPin, OUTPUT);
  dht.begin();
  digitalWrite(buzzerPin, LOW);
  digitalWrite(lightPin, LOW);
  button1.attachClick(changeBuzzerState);
  button2.attachClick(changeLightState);
  Serial.println("Sender is ready.");
  esp_task_wdt_init(2, true); 
  esp_task_wdt_add(NULL);
}

void loop() {
  esp_task_wdt_reset();

  button1.tick();
  button2.tick();
  receiveCommand();
  delay(10);

  static unsigned long lastSensorRead = 0;
  static unsigned long lastStatusSend = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - lastSensorRead >= 1000) {
    lastSensorRead = currentMillis;
    readSensor();
  }

  if (currentMillis - lastStatusSend >= 1000) {
    lastStatusSend = currentMillis;
    sendStatus();
  }
}

void changeBuzzerState() {
  buzzerState = !buzzerState;
  digitalWrite(buzzerPin, buzzerState ? HIGH : LOW);
  Serial.println("Button pressed, buzzer state: " + String(buzzerState ? "ON" : "OFF"));
}
void changeLightState() {
  lightState = !lightState;
  digitalWrite(lightPin, lightState ? HIGH : LOW);
  Serial.println("Button pressed, light state: " + String(lightState ? "ON" : "OFF"));
}
void readSensor() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  mq2 = analogRead(mq2Pin);
  lux = lightMeter.readLightLevel();
}

void sendStatus() {
  String buzzerStateStr = buzzerState ? "BUKITON" : "BUKITOFF";
  String lightStateStr = lightState ? "LIKITON" : "LIKITOFF";
  String message = String("Buzzer: ") + buzzerStateStr + " ,"+
                  String("Light: ") + lightStateStr +
                   ", Temp: " + String(temperature) + "C" +
                   ", Humidity: " + String(humidity) + "%" +
                   ", mq2: " + String(mq2) +
                   ", lux: " + String(lux) +
                   " From Add: " + local_add;
  zigbeeSerial.println(message);
  Serial.println("Sent: " + message);
}

void receiveCommand() {
  if (zigbeeSerial.available()) {
    String command = zigbeeSerial.readStringUntil('\n');
    command.trim();
    Serial.println("Received command: " + command);
    if (command == "BUKITON") {
      buzzerState = 1;
      digitalWrite(buzzerPin, HIGH);
    } else if (command == "BUKITOFF") {
      buzzerState = 0;
      digitalWrite(buzzerPin, LOW);
    }else if (command == "LIKITON") {
      lightState = 1;
      digitalWrite(lightPin, HIGH);
    }else if (command == "LIKITOFF") {
      lightState = 0;
      digitalWrite(lightPin, LOW);
    }
  }
}
