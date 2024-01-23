#ifndef GETREADING_H
#define GETREADING_H

#include <Arduino_JSON.h>
#include <DallasTemperature.h>

#include <FS.h>

void appendFile(fs::FS &fs, const char *path, const char *message);

extern JSONVar readings;
extern DeviceAddress sensor1;
extern DeviceAddress sensor2;

String getSensorReadings();

#endif  // GETREADING_H
