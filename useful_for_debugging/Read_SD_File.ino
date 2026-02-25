/*
  SD card file dump

  This example shows how to read a file from the SD card using the
  SD library and send it over the serial port.
  */

String fxname = "cDatat0.csv"; // WRITE YOUR FILE NAME

#include <SD.h>


#define SD_CS 6

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  // wait for Serial Monitor to connect. Needed for native USB port boards only:
  while (!Serial);

  if (!SD.begin(SD_CS)) {
    Serial.println("initialization failed. Things to check:");
    // Serial.println("1. is a card inserted?");
    // Serial.println("2. is your wiring correct?");
    // Serial.println("3. did you change the chipSelect pin to match your shield or module?");
    // Serial.println("Note: press reset button on the board and reopen this Serial Monitor after fixing your issue!");
  }

  Serial.println("initialization done.");

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open(fxname);

  // if the file is available, write to it:
  if (dataFile) {
    while (dataFile.available()) {
      Serial.write(dataFile.read());
    }
    dataFile.close();
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening your file");
  }
}

void loop() {
}
