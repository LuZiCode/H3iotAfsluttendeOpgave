#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Arduino_JSON.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <AsyncElegantOTA.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "config.h" // Custom config
#include "sdCardLogic.h" // SD card Logic 
#include "getReading.h" // Reading Logic

// Define pin for WIFI Reset button
const int buttonPin = RESET_BTN_VALUE; // Erstat med din GPIO pin
unsigned long buttonPressTime = 0;
bool isButtonPressed = false;

const int oneWireBus = ONE_WIRE_BUS_VALUE;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
OneWire ds(ONE_WIRE_BUS_VALUE);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

extern JSONVar readings;

unsigned long lastTime = 0;
unsigned long timerDelay = 15000;

String ssidString;
String pass;
String ip;
String gateway;

const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "gateway";

const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";
const char* gatewayPath = "/gateway.txt";

IPAddress dns(8, 8, 8, 8);
IPAddress localIP;
IPAddress localGateway;
IPAddress subnet(255, 255, 0, 0);

const long interval = 10000;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "87.104.58.9", 3600, 60000);

String formattedDate;
String dayStamp;
String timeStamp;

// SD CARD ############################################################################################################################

void getTimeStamp() {
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  formattedDate = timeClient.getFormattedDate();
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  timeStamp = formattedDate.substring(splitT + 1, formattedDate.length() - 1);
}

void sendSensorReadingsToWebSocket() {
  getTimeStamp();
  String jsonString = getSensorReadings();
  JSONVar json = JSON.parse(jsonString);
  json["time"] = timeStamp;
  jsonString = JSON.stringify(json);
  ws.textAll(jsonString);

  // Log data to SD Card
  int maxReadingID = getMaxReadingID();
  logSDCard(sensors, maxReadingID + 1, dayStamp, timeStamp);
}


// LittleFS #############################################################################################################################

void initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
}

String readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return String();
  }
  String fileContent;
  while (file.available()) {
    fileContent = file.readStringUntil('\n');
    break;
  }
  return fileContent;
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
}

bool initWiFi() {
  if (ssidString == "" || ip == "") {
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  localIP.fromString(ip.c_str());
  localGateway.fromString(gateway.c_str());

  if (!WiFi.config(localIP, localGateway, subnet)) {
    Serial.println("STA Failed to configure");
    return false;
  }
  WiFi.begin(ssidString.c_str(), pass.c_str());
  Serial.println("Connecting to WiFi...");

  unsigned long currentMillis = millis();
  unsigned long previousMillis = currentMillis;

  while (WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      Serial.println("Failed to connect.");
      return false;
    }
  }

  Serial.println(WiFi.localIP());
  return true;
}

void setup() {
  Serial.begin(115200);
  initWiFi();
  initLittleFS();
  initSDCard(); // Initialize SD Card

  ssidString = readFile(LittleFS, ssidPath);
  pass = readFile(LittleFS, passPath);
  ip = readFile(LittleFS, ipPath);
  gateway = readFile(LittleFS, gatewayPath);

  if (initWiFi()) {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(LittleFS, "/index.html", "text/html", false);
    });
    server.serveStatic("/", LittleFS, "/");

    server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request) {
      String jsonString = getSensorReadings();
      JSONVar json = JSON.parse(jsonString);
      json["time"] = timeStamp;
      jsonString = JSON.stringify(json);
      request->send(200, "application/json", jsonString);
      Serial.println("XXXXXX");
      Serial.println(timeStamp);
      Serial.println(jsonString);
    });

    server.addHandler(&ws);
    server.begin();
  } else {
    WiFi.softAP(WIFI_SSID, NULL);
    IPAddress IP = WiFi.softAPIP();
    Serial.println(IP);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(LittleFS, "/wifimanager.html", "text/html");
    });

    server.serveStatic("/", LittleFS, "/");

    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for (int i = 0; i < params; i++) {
        AsyncWebParameter *p = request->getParam(i);
        if (p->isPost()) {
          if (p->name() == PARAM_INPUT_1) {
            ssidString = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssidString);
            writeFile(LittleFS, ssidPath, ssidString.c_str());
          }
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            writeFile(LittleFS, passPath, pass.c_str());
          }
          if (p->name() == PARAM_INPUT_3) {
            ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(ip);
            writeFile(LittleFS, ipPath, ip.c_str());
          }
          if (p->name() == PARAM_INPUT_4) {
            gateway = p->value().c_str();
            Serial.print("Gateway set to: ");
            Serial.println(gateway);
            writeFile(LittleFS, gatewayPath, gateway.c_str());
          }
        }
      }
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
      delay(3000);
      ESP.restart();
    });
    server.addHandler(&ws);
    server.begin();
  }
  pinMode(buttonPin, INPUT_PULLUP);
}

void loop() {
  timeClient.update();
  if ((millis() - lastTime) > timerDelay) {
    sendSensorReadingsToWebSocket();
    lastTime = millis();
  }
  // Check Resetbtn status
  if (digitalRead(buttonPin) == LOW) {
    if (!isButtonPressed) {
      isButtonPressed = true;
      buttonPressTime = millis();
    } else if (millis() - buttonPressTime > 5000) {
      // Nulstil WiFi-konfigurationen
      Serial.println("Resseting WiFi-Configuration...");
      LittleFS.remove(ssidPath);
      LittleFS.remove(passPath);
      LittleFS.remove(ipPath);
      LittleFS.remove(gatewayPath);

      // Genstart ESP32
      ESP.restart();
    }
  } else {
    isButtonPressed = false;
  }
}
