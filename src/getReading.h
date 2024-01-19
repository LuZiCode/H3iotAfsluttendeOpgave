#ifndef GET_READING_H
#define GET_READING_H

#include <OneWire.h>
#include <DallasTemperature.h>
#include <Arduino_JSON.h>

extern DallasTemperature sensors;
extern DeviceAddress sensor1;
extern DeviceAddress sensor2;

String getSensorReadings();

#endif  // GET_READING_H
