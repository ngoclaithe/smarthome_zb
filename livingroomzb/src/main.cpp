#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <HardwareSerial.h>
#include "OneButton.h"
#include <DHT.h>
#include "esp_task_wdt.h"

#define DHTTYPE DHT11

String local_add = "0xbb";
int nutnhan1 = 5;
const int lightPin = 15;
const int dhtPin = 14;
const int pirPin = 18;
const int buzzerPin = 19;
bool lightState = false;
float temperature = 0.0;
float humidity = 0.0;
int pirState = 0;
bool buzzerState = false;

OneButton button1(nutnhan1, true);
HardwareSerial zigbeeSerial(2);
DHT dht(dhtPin, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

void changeLightState();
void readSensor();
void sendStatus();
void receiveCommand();

void setup() {
  Serial.begin(9600);
  zigbeeSerial.begin(9600, SERIAL_8N1, 17, 16);
  pinMode(lightPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(pirPin, INPUT);
  dht.begin();
  lcd.init();
  lcd.backlight();
  digitalWrite(buzzerPin, LOW);
  button1.attachClick(changeLightState);
  Serial.println("Sender is ready.");

  esp_task_wdt_init(2, true);
  esp_task_wdt_add(NULL);
}

void loop() {
  esp_task_wdt_reset();

  button1.tick();
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

void changeLightState() {
  lightState = !lightState;
  digitalWrite(lightPin, lightState ? HIGH : LOW);
  Serial.println("Button pressed, light state: " + String(lightState ? "ON" : "OFF"));
}

void readSensor() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  pirState = digitalRead(pirPin) == HIGH ? 1 : 0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("Humidity: ");
  lcd.print(humidity);
  lcd.print("%");
}

void sendStatus() {
  String message = String("Light: ") + (lightState ? "ON" : "OFF") +
                   ", Temp: " + String(temperature) + "C" +
                   ", Humidity: " + String(humidity) + "%" +
                   ", PIR: " + String(pirState) +
                   ", Buzzer: " + (buzzerState ? "BUZZON" : "BUZZOFF") +
                   " From Add: " + local_add;
  zigbeeSerial.println(message);
  Serial.println("Sent: " + message);
}

void receiveCommand() {
  if (zigbeeSerial.available()) {
    String command = zigbeeSerial.readStringUntil('\n');
    command.trim();
    Serial.println("Received command: " + command);
    if (command == "ON") {
      lightState = true;
      digitalWrite(lightPin, HIGH);
    } else if (command == "OFF") {
      lightState = false;
      digitalWrite(lightPin, LOW);
    } else if (command == "BUZZON") {
      buzzerState = true;
      digitalWrite(buzzerPin, HIGH);
    } else if (command == "BUZZOFF") {
      buzzerState = false;
      digitalWrite(buzzerPin, LOW);
    }
  }
}
