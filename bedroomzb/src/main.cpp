#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <HardwareSerial.h>
#include "OneButton.h"
#include <DHT.h>
#include "esp_task_wdt.h"

#define DHTTYPE DHT11

String local_add = "0xba";
int nutnhan1 = 5;
int nutnhan2 = 18;
const int fanPin = 19;
const int lightPin = 13;
const int dhtPin = 14;
bool lightState = false;
bool fanState = false;
float temperature = 0.0;
float humidity = 0.0;
int brightness = 0; 

OneButton button1(nutnhan1, true);
OneButton button2(nutnhan2, true);
HardwareSerial zigbeeSerial(2);
DHT dht(dhtPin, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

void changeLightState();
void readSensor();
void sendStatus();
void receiveCommand();
void changeFanState();
void changeLightState();

void setup() {
  Serial.begin(9600);
  zigbeeSerial.begin(9600, SERIAL_8N1, 16, 17);
  pinMode(lightPin, OUTPUT);
  pinMode(fanPin, OUTPUT);
  dht.begin();
  lcd.init();
  lcd.backlight();
  button1.attachClick(changeLightState);
  button2.attachClick(changeFanState);
  Serial.println("Sender is ready.");
  digitalWrite(fanPin, LOW);
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

void changeLightState() {
  lightState = !lightState;
  brightness = lightState ? 255 : 0; 
  analogWrite(lightPin, brightness);
  Serial.println("Button 1 pressed, light state: " + String(lightState ? "ON" : "OFF") + ", brightness: " + String(brightness));
}

void changeFanState() {
  fanState = !fanState;
  digitalWrite(fanPin, fanState ? HIGH : LOW);
  Serial.println("Button 2 pressed, fan state: " + String(fanState ? "ON" : "OFF"));
}

void readSensor() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
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
  String message = String("Light: ") + (lightState ? "LIGHTON" : "LIGHTOFF") + "," +
                  String("Brightness: ") + String(brightness) + "," + 
                  String("Fan: ") + (fanState ? "FANON" : "FANOFF") +
                   ", Temp: " + String(temperature) + "C" +
                   ", Humidity: " + String(humidity) + "%" +
                   " From Add: " + local_add;
  zigbeeSerial.println(message);
  Serial.println("Sent: " + message);
}
void receiveCommand() {
  if (zigbeeSerial.available()) {
    String command = zigbeeSerial.readStringUntil('\n');
    command.trim();
    Serial.println("Received command: " + command);

    if (command.startsWith("LIGHTON")) {
      lightState = true;
      brightness = command.substring(8).toInt();
      analogWrite(lightPin, brightness);
    } else if (command.startsWith("LIGHTOFF")) {
      lightState = false;
      brightness = 0;
      analogWrite(lightPin, brightness);
    } else if (command == "FANON") {
      fanState = true;
      digitalWrite(fanPin, HIGH);
    } else if (command == "FANOFF") {
      fanState = false;
      digitalWrite(fanPin, LOW);
    }
  }
}
