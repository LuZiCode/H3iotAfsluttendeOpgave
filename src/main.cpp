// Include necessary libraries for WiFi, web server, file system, JSON parsing, OneWire, DallasTemperature, OTA updates, NTP client
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

// Include custom configuration and logic files
#include "config.h" // Custom config
#include "sdCardLogic.h" // SD card Logic 
#include "getReading.h" // Reading Logic

// Function prototypes for handling button interactions and resetting WiFi configuration
bool buttonHandling(String method);
void resetWifiConfiguration();

// Pin configuration for WiFi Reset button
const int buttonPin = RESET_BTN_VALUE; // Replace with your GPIO pin
unsigned long buttonPressTime = 0;
bool isButtonPressed = false;

// OneWire and DallasTemperature setup for sensor reading
const int oneWireBus = ONE_WIRE_BUS_VALUE;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
OneWire ds(ONE_WIRE_BUS_VALUE);

// Web server and WebSocket setup 
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Global variables for sensor readings and time management
extern JSONVar readings;
unsigned long lastTime = 0;
unsigned long timerDelay = 15000;

// WiFi configuration variables
String ssidString;
String pass;
String ip;
String gateway;

// Constants for handling POST requests
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "gateway";

// File paths for WiFi configuration stored in LittleFS
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";
const char* gatewayPath = "/gateway.txt";

// Network configuration for static IP
IPAddress dns(8, 8, 8, 8);
IPAddress localIP;
IPAddress localGateway;
IPAddress subnet(255, 255, 0, 0);

// NTP client for time synchronization
const long interval = 10000;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "87.104.58.9", 3600, 60000);

// Variables to store formatted date and time
String formattedDate;
String dayStamp;
String timeStamp;

// Function to get the current timestamp from NTP server
bool getTimeStamp() {
  // Update timeClient and wait for a response
  if (!timeClient.update()) {
    timeClient.forceUpdate();
    delay(500); // Rest of the function handles time synchronization and parsing
  }

  // Tjek igen efter forceUpdate
  if (!timeClient.update()) {
    return false; // Returner false, hvis tiden stadig ikke er opdateret
  }

  // Uddrag dato og tid
  formattedDate = timeClient.getFormattedDate();
  int splitT = formattedDate.indexOf("T");
  if (splitT == -1) {
    return false; // Returner false, hvis formateringen er forkert
  }

  dayStamp = formattedDate.substring(0, splitT);
  timeStamp = formattedDate.substring(splitT + 1, formattedDate.length() - 1);

  return true; // Gyldig tidspunkt modtaget
}

// Function to send sensor readings to WebSocket and log data on SD card
void sendSensorReadingsToWebSocket() {
  if (getTimeStamp()) {
    String jsonString = getSensorReadings();
    JSONVar json = JSON.parse(jsonString);
    json["time"] = timeStamp;
    jsonString = JSON.stringify(json);

    // Ensure that the String is UTF-8 encoded
    ws.textAll(String (jsonString.c_str()));

    // For debugging, print the UTF-8 encoded data
    Serial.println(jsonString);

    // Log data to SD card
    int maxReadingID = getMaxReadingID();
    logSDCard(sensors, maxReadingID + 1, dayStamp, timeStamp);
  } else {
    Serial.println("Invalid timestamp, skipping logging");
    // Add additional actions if the timestamp is invalid
  }
}


// LittleFS #############################################################################################################################

// Function to initialize LittleFS
void initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
}

// Function to read a file from filesystem
String readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path);
  //If no file run this if statement
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return String();
  }
  String fileContent;
  // While file is available
  while (file.available()) {
    //Set variable to file until a new line
    fileContent = file.readStringUntil('\n');
    break;
  }
  //Return file content
  return fileContent;
}

// Function to write to a file in filesystem
void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, FILE_WRITE);
  //if fail to open filepath
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

// Function to initialize WiFi connection
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

// Function to handle WebSocket messages
void handleWebSocketMessage(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  // Handle WebSocket messages here
  if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      // Handle text message received from WebSocket client
      String message = "";
      for(size_t i = 0; i < len; i++) {
        message += (char)data[i];
      }
      // Process the WebSocket message as needed
      Serial.println("WebSocket Message received: " + message);
    }
  }
}

void setup() {
  Serial.begin(115200);

  initWiFi();
  initLittleFS();
  initSDCard(); // Initialize SD Card
  ws.onEvent(handleWebSocketMessage);

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
    });

    server.on("/loaddata", HTTP_GET, [](AsyncWebServerRequest *request) {
      // Read historical data from the SD card file
      File file = SD.open("/data.txt", FILE_READ); // Assuming the data is stored in "/data.txt"
      if (file) {
        String historicalData = file.readString();
        file.close();
        // Send the historical data as a response
        request->send(200, "application/json", historicalData);
      } else {
        request->send(404, "text/plain", "File not found");
      }
    });

    server.on("/buttonHandling", HTTP_GET, [](AsyncWebServerRequest *request) {
      //If /buttonHandling has a parameter called "param" run this if statement
      if (request->hasArg("param")) {
        String paramValue = request->arg("param");
        //if buttonHandling returns true
        if (buttonHandling(paramValue)) {
          //if parameter is equal to "csvdownload"
          if (paramValue == "csvdownload") {
            //send the content of SD card through the response as text/plain
            request->send(SD, "/data.txt", "text/plain");
          } else { // else run this
            //send back a 200 response, text/plain, with a success message
            request->send(200, "text/plain", paramValue + "successful");
          }
        } else {
          //else send 400 reponse type with an error message
          request->send(400, "text/plain", "Noget gik galt under buttonHandling");
        }
      // else run this
      } else {
        request->send(400, "text/plain", "Parameter 'param' mangler");
      }
    });

    server.addHandler(&ws);
    server.begin();
  } else {
    WiFi.softAP(WIFI_SSID, NULL);
    IPAddress IP = WiFi.softAPIP();
    Serial.println(IP);

    //if server is accessed be default domain run this
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
  //update timeclient
  timeClient.update();
  if ((millis() - lastTime) > timerDelay) {
    sendSensorReadingsToWebSocket();
    lastTime = millis();
  }
  // Check Resetbtn status
  if (digitalRead(buttonPin) == LOW) {
    //if variable is false
    if (!isButtonPressed) {
      isButtonPressed = true;
      buttonPressTime = millis();
    } else if (millis() - buttonPressTime > 5000) { //if button is pressed for 5 seconds
      //reset wifi conf
      resetWifiConfiguration();
    }
  } else {
    isButtonPressed = false;
  }
}

// Resets WiFi configuration by removing saved settings and restarting ESP32
void resetWifiConfiguration() {
  Serial.println("Resetting WiFi Configuration...");
  LittleFS.remove(ssidPath);
  LittleFS.remove(passPath);
  LittleFS.remove(ipPath);
  LittleFS.remove(gatewayPath);

  // Restart ESP32
  ESP.restart();
}

// Handles different button interactions based on the 'method' parameter
bool buttonHandling(String method) {
  if (method == "clearnetwork") {
    Serial.println("clearnetwork if statement hit");
    resetWifiConfiguration();
    return true;
  } else if (method == "csvdownload") {
    Serial.println("csvdownload if statement hit");
    // return downloadCSV method
    return downloadCSV();
  } else if (method == "deletedata") {
    // return clearDatafile method
    return clearDataFile();
  }
  //if none of the if statements if hit, return false
  return false;
}