#include "sdCardLogic.h"
#include "getReading.h"
#include "config.h"

// SD Card related variables
const int SD_CS = SD_CS_VALUE;
RTC_DATA_ATTR int readingID = 0;
extern JSONVar readings;

void initSDCard() {
  //if no sd card on the sd pin value run this
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card Mount Failed");
    return;
  }
}

//Get the current maxreadingid on sd card
int getMaxReadingID() {
  File file = SD.open("/data.txt", FILE_READ);
  int maxReadingID = 0;

  if (file) {
    while (file.available()) {
      String line = file.readStringUntil('\n');
      int currentReadingID = line.substring(0, line.indexOf(',')).toInt();
      if (currentReadingID > maxReadingID) {
        maxReadingID = currentReadingID;
      }
    }
    file.close();
  }

  return maxReadingID;
}

//log data to sd card
void logSDCard(DallasTemperature& sensors, int currentReadingID, String dayStamp, String timeStamp) {
    sensors.begin();
    getSensorReadings();

    readingID = currentReadingID;
    Serial.print("Current Reading ID: ");
    Serial.println(readingID);
    // Tjek om timeStamp er gyldig
    if (timeStamp.length() > 5 && timeStamp.indexOf(":") > 0) {
        String dataMessage = String(readingID) + "," + dayStamp + "," + timeStamp + "," +
                             JSON.stringify(readings["sensor1"]) + "," +
                             JSON.stringify(readings["sensor2"]) + "\r\n";
        Serial.print("Save data: ");
        Serial.println(dataMessage);
        appendFile(SD, "/data.txt", dataMessage.c_str());
    } else {
        Serial.println("Ugyldigt timestamp, gemmer ikke data");
    }
}

//clear data on the sd card
bool clearDataFile() {
  File file = SD.open("/data.txt", FILE_WRITE);
  if (file) {
    Serial.println("Data file cleared");
    file.close();
    return true;
  } else {
    Serial.println("Failed to open data file for clearing");
    return false;
  }
  return false;
}

//check if sd card is ready to be downloaded
bool downloadCSV() {
    if (SD.exists("/data.txt")) {
        Serial.println("Data file is ready for download.");
        return true;
    } else {
        Serial.println("Data file does not exist.");
        return false;
    }
}


