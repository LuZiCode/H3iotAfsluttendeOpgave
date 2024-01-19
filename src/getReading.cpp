#include "getReading.h"

// Json Variable to Hold Sensor Readings
JSONVar readings;

// Address of each sensor
DeviceAddress sensor1 = {0x28, 0xFF, 0x64, 0x1E, 0x1, 0xD8, 0xDC, 0x1A};
DeviceAddress sensor2 = {0x28, 0xFF, 0x64, 0x1E, 0x30, 0x67, 0x1, 0xDF};

String getSensorReadings() {
  sensors.requestTemperatures();
  float temp1 = sensors.getTempC(sensor1);
  float temp2 = sensors.getTempC(sensor2);

  Serial.print("Sensor 1 Temperature: ");
  Serial.println(temp1);
  Serial.print("Sensor 2 Temperature: ");
  Serial.println(temp2);

  readings["sensor1"] = String(temp1);
  readings["sensor2"] = String(temp2);

  String jsonString = JSON.stringify(readings);
  return jsonString;
}
