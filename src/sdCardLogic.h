#ifndef SDCARDLOGIC_H
#define SDCARDLOGIC_H

#include <DallasTemperature.h>
#include <SD.h>
#include <SPI.h>
#include <OneWire.h>

void initSDCard();
int getMaxReadingID();
void logSDCard(DallasTemperature& sensors, int currentReadingID, String dayStamp, String timeStamp);
bool clearDataFile();
bool downloadCSV();
#endif // SDCARDLOGIC_H
