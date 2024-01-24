#include "sdCardLogic.h"
#include "getReading.h"
#include "config.h"

// SD Card related variables
const int SD_CS = SD_CS_VALUE;
RTC_DATA_ATTR int readingID = 0;
extern JSONVar readings;

void initSDCard() {
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card Mount Failed");
    return;
  }
}

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

