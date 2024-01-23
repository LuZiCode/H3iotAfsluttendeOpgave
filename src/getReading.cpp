#include "getReading.h"
#include <OneWire.h>
#include <DallasTemperature.h>

extern OneWire oneWire;
extern DallasTemperature sensors;

// Array to hold device addresses
DeviceAddress sensorAddresses[2]; 
int numberOfSensors = 0;
bool sensorsInitialized = false;

// Function to initialize and find sensor addresses
void findSensors() {
  sensors.begin();
  numberOfSensors = 0;
  while (sensors.getAddress(sensorAddresses[numberOfSensors], numberOfSensors)) {
    numberOfSensors++;
    if (numberOfSensors == 3) break; // Prevent array overflow
  }
  sensors.setResolution(12); // Optional: Set resolution for all sensors
  sensorsInitialized = true;
}

// Function to get sensor readings
String getSensorReadings() {
  if (!sensorsInitialized) {
    findSensors(); // Find sensors if not already done
  }

  sensors.requestTemperatures();

  JSONVar readings;
  for (int i = 0; i < numberOfSensors; i++) {
    float temp = sensors.getTempC(sensorAddresses[i]);
    
    Serial.print("Sensor ");
    Serial.print(i + 1);
    Serial.print(" Temperature: ");
    Serial.println(temp);

    readings["sensor" + String(i + 1)] = String(temp);
  }

  String jsonString = JSON.stringify(readings);
  return jsonString;
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
    File file = fs.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for appending");
        return;
    }
    file.print(message);
    file.close();
}