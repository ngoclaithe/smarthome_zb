#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* ssid = "tang2";
const char* password = "12345678";
const char* mqtt_server = "192.168.31.254";
const char* mqtt_user = "mqtt_user";
const char* mqtt_pass = "12345678";

WiFiClient espClient;
PubSubClient client(espClient);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool nodeConnected[3] = { false };
unsigned long previousDisplayTime = 0;
const unsigned long displayInterval = 2000;

int brightnessLevel = 0;

void displayConnectionStatus(String nodeAddress) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (nodeAddress == "0xbb") {
    display.setCursor(0, 0);
    display.println("0xbb is online.");
  } else if (nodeAddress == "0xba") {
    display.setCursor(0, 15);
    display.println("0xba is online.");
  } else if (nodeAddress == "0xbc") {
    display.setCursor(0, 30);
    display.println("0xbc is online.");
  }

  display.display();
}

void handleSensorDataFromBB(String message) {
  DynamicJsonDocument doc(256);
  int tempIndex = message.indexOf("Temp: ");
  int humiIndex = message.indexOf("Humidity: ");
  int lightIndex = message.indexOf("Light: ");
  int pirIndex = message.indexOf("PIR: ");
  int buzzerIndex = message.indexOf("Buzzer: ");
  if (tempIndex != -1 && humiIndex != -1 && lightIndex != -1 && pirIndex != -1) {
    String tempStr = message.substring(tempIndex + 6, message.indexOf("C", tempIndex));
    String humiStr = message.substring(humiIndex + 10, message.indexOf("%", humiIndex));
    String light = message.substring(lightIndex + 7, message.indexOf(",", lightIndex));
    String pir = message.substring(pirIndex + 5, message.indexOf(",", pirIndex + 5));
    String buzzer = message.substring(buzzerIndex + 8, message.indexOf(" ", buzzerIndex + 8));

    float temp = tempStr.toFloat();
    float humi = humiStr.toFloat();
    doc["temperature"] = temp;
    doc["humidity"] = humi;
    doc["light"] = light;
    doc["pir"] = pir;
    doc["buzzer"] = buzzer;
  }

  char buffer[256];
  size_t n = serializeJson(doc, buffer);
  client.publish("home-assistant/sensor1", buffer, n);

  nodeConnected[0] = true;
  displayConnectionStatus("0xbb");
}

void handleSensorDataFromBA(String message) {
  DynamicJsonDocument doc(256);
  int tempIndex = message.indexOf("Temp: ");
  int humiIndex = message.indexOf("Humidity: ");
  int lightIndex = message.indexOf("Light: ");
  int fanIndex = message.indexOf("Fan: ");
  int brightnessIndex = message.indexOf("Brightness: ");

  if (tempIndex != -1 && humiIndex != -1 && lightIndex != -1 && fanIndex != -1 && brightnessIndex != -1) {
    String tempStr = message.substring(tempIndex + 6, message.indexOf("C", tempIndex));
    String humiStr = message.substring(humiIndex + 10, message.indexOf("%", humiIndex));
    String light = message.substring(lightIndex + 7, message.indexOf(",", lightIndex));
    String fan = message.substring(fanIndex + 5, message.indexOf(",", fanIndex));
    String brightnessStr = message.substring(brightnessIndex + 12, message.indexOf(",", brightnessIndex));
    
    float temp = tempStr.toFloat();
    float humi = humiStr.toFloat();
    int brightness = brightnessStr.toInt();
    
    doc["temperature"] = temp;
    doc["humidity"] = humi;
    doc["light"] = light;
    doc["fan"] = fan;
    doc["brightness"] = brightness;
  }

  char buffer[256];
  size_t n = serializeJson(doc, buffer);
  client.publish("home-assistant/sensor2", buffer, n);

  nodeConnected[1] = true;
  displayConnectionStatus("0xba");
}

void handleSensorDataFromBC(String message) {
  DynamicJsonDocument doc(256);
  int buzzerIndex = message.indexOf("Buzzer: ");
  int lightIndex = message.indexOf("Light: ");
  int tempIndex = message.indexOf("Temp: ");
  int humiIndex = message.indexOf("Humidity: ");
  int mq2Index = message.indexOf("mq2: ");
  int luxIndex = message.indexOf("lux: ");

  if (buzzerIndex != -1 && lightIndex != -1 && tempIndex != -1 && humiIndex != -1 && mq2Index != -1 && luxIndex != -1) {
    String buzzerStateStr = message.substring(buzzerIndex + 8, message.indexOf(",", buzzerIndex));
    String lightStateStr = message.substring(lightIndex + 7, message.indexOf(",", lightIndex));
    String tempStr = message.substring(tempIndex + 6, message.indexOf("C", tempIndex));
    String humiStr = message.substring(humiIndex + 10, message.indexOf("%", humiIndex));
    String mq2Str = message.substring(mq2Index + 5, message.indexOf(",", mq2Index));
    String luxStr = message.substring(luxIndex + 5, message.indexOf(" ", luxIndex + 5));

    doc["buzzerState"] = buzzerStateStr;
    doc["lightState"] = lightStateStr;
    doc["temperature"] = tempStr.toFloat();
    doc["humidity"] = humiStr.toFloat();
    doc["mq2"] = mq2Str.toInt();
    doc["lux"] = luxStr.toFloat();
  }

  char buffer[256];
  size_t n = serializeJson(doc, buffer);
  client.publish("home-assistant/sensor3", buffer, n);

  nodeConnected[2] = true;
  displayConnectionStatus("0xbc");
}

void sendCommandToZigbee(String command) {
  if (command.startsWith("LIGHTON") || command.startsWith("LIGHTOFF")) {
    command += " " + String(brightnessLevel);
  }
  Serial2.println(command);
  Serial.println("Command sent to Zigbee: " + command);
}

void setup_wifi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }
  Serial.println(messageTemp);

  if (String(topic) == "zigbee/command") {
    sendCommandToZigbee(messageTemp);
  } else if (String(topic) == "zigbee/command/brightness") {
    brightnessLevel = messageTemp.toInt();
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      client.subscribe("zigbee/command");
      client.subscribe("zigbee/command/brightness");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, 17, 16);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.display();
  display.clearDisplay();

  Serial.println("Receiver is ready.");
}

void sendSensorData(String message) {
  if (message.indexOf("From Add: 0xbb") != -1) {
    handleSensorDataFromBB(message);
  } else if (message.indexOf("From Add: 0xba") != -1) {
    handleSensorDataFromBA(message);
  } else if (message.indexOf("From Add: 0xbc") != -1) {
    handleSensorDataFromBC(message);
  } else {
    Serial.println("Unknown address in message: " + message);
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (Serial2.available()) {
    String message = Serial2.readStringUntil('\n');
    Serial.println(message);
    sendSensorData(message);
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousDisplayTime >= displayInterval) {
    previousDisplayTime = currentMillis;
    for (int i = 0; i < 3; ++i) {
      if (nodeConnected[i]) {
        displayConnectionStatus(i == 0 ? "0xbb" : (i == 1 ? "0xba" : "0xbc"));
        nodeConnected[i] = false;
        display.clearDisplay();
        display.display();
        break;
      }
    }
  }
}
