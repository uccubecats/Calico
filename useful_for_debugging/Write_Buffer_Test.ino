// Camera and SD Test File
// Test file to capture image and write it to SD card using ArduCAM OV2640 and Arduino Mega

#include <SPI.h>
#include <SD.h>

#define SD_CS 6
#define MAX_LINES 1000



struct DataPacket {
  uint8_t second, minute, hour;
  uint32_t latitude, longitude, altitude;
  uint8_t satellites, fixtype;
  uint32_t NO2, C2H5OH, VOC, CO;
  uint16_t ozone;
  float pressure, uv, lux, temperature, humidity;
};

bool writeDataPacket(DataPacket*, size_t);
bool writeDataPacket(DataPacket, File);


uint32_t lineCount = 0;

void setup() {
  Serial.begin(9600);
  const size_t BUFSIZE = 10;
  DataPacket dummyBuffer[BUFSIZE];
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, LOW); // Turn on
  SD.begin(SD_CS);

  // Fill with dummy data
  for (size_t i = 0; i < BUFSIZE; ++i) {
    dummyBuffer[i] = {
      (uint8_t)(i + 1), 10, 12,
      10000000 + i, 20000000 + i, 10000 + i,
      8, 3,
      50 + i, 60 + i, 70 + i, 80 + i, 400 + i,
      1013.25 + i, 5.0 + i, 100.0 + i,
      25.0 + i, 50.0 + i
    };
  }

  writeDataPacket(dummyBuffer, BUFSIZE);
}


void loop() {
  // Nothing here
}

bool writeDataPacket(DataPacket* packets, size_t packetCount) {
  static int fileIndex = 0;
  static String filename;

  // Handle Header for New Files
  if (lineCount == 0 || lineCount >= MAX_LINES) {
    filename = "cData" + String(fileIndex++) + ".csv";
    File dataFile = SD.open(filename.c_str(), FILE_WRITE);
    if (dataFile) {
      dataFile.println(
        "second,minute,hour,"
        "latitude,longitude,altitude,satellites,fixtype,"
        "NO2,C2H5OH,VOC,CO,ozone,pressure,uv,lux,temperature,humidity"
      );
      dataFile.close();
      Serial.println("Written Head");
    }
    lineCount = 0;
  }

  File dataFile = SD.open(filename, FILE_WRITE);
  if (!dataFile) return false;

  for (size_t i = 0; i < packetCount; ++i) {
    if (!writeDataPacket(packets[i], dataFile)) {
      dataFile.close();
      return false;
    }
    // String a = "Written Line" + String(i);
    // Serial.println(a);
  }

  dataFile.close();
  return true;
}


// Assumes SD is ON
bool writeDataPacket(DataPacket packet, File dataFile) {
  if (!dataFile) return false;
  dataFile.print(packet.second);        dataFile.print(",");
  dataFile.print(packet.minute);        dataFile.print(",");
  dataFile.print(packet.hour);          dataFile.print(",");

  dataFile.print(packet.latitude);      dataFile.print(",");
  dataFile.print(packet.longitude);     dataFile.print(",");
  dataFile.print(packet.altitude);      dataFile.print(",");
  dataFile.print(packet.satellites);    dataFile.print(",");
  dataFile.print(packet.fixtype);       dataFile.print(",");

  dataFile.print(packet.NO2);           dataFile.print(",");
  dataFile.print(packet.C2H5OH);        dataFile.print(",");
  dataFile.print(packet.VOC);           dataFile.print(",");
  dataFile.print(packet.CO);            dataFile.print(",");
  dataFile.print(packet.ozone);         dataFile.print(",");
  dataFile.print(packet.pressure);      dataFile.print(",");
  dataFile.print(packet.uv);            dataFile.print(",");
  dataFile.print(packet.lux);           dataFile.print(",");
  dataFile.print(packet.temperature);   dataFile.print(",");
  dataFile.println(packet.humidity);
  lineCount++;
  return true;
}