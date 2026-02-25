/*
Project Calico - CubeCats CATiSE Program Fall 2024-Spring 2025
 * Main Mission Code
 * For Wiring Diagram and Extra Notes, See Wiring_and_Notes.txt
* User Digital Inputs to Arudino:
    Iridium Serial: Serial3
    Temperaute Sensor Digital Pin: 7
    Camera Digital Pin: 6
    Led Digital Pin: 5


*/

// Mission Parameters (Time in seconds)
  #define MAIN_BAUD 115200          // Baud rate used for Serial Monitor Printing
  #define SENSOR_INTERVAL 1       // Minimum interval between sensor collection
  #define AVERAGING_INTERVAL 45   // Minimum Span to average a packet over
  #define MESSAGE_INTERVAL 180    // Minimum interval between Sending Messages to Satellite FIXME: Correct back to 180 seconds
  #define PICTURE_INTERVAL 180   // Minimum interval between taking a picture for local storage 
  #define CALLBACK_INTERVALS_COEFFICIENT 2 // The constant by which all intervals all multiplied by for the callback function in the satellite transmission
  #define PACKETS_PER_MESSAGE 4   // Number of Packets in every message to be sent to the sattelite (proportional to max size of message 340, and packet size)
  #define LINES_BUFFER_SIZE 10    // Number of Sensor readings before a write to buffer is necessary
  #define MESSAGE_SIZE 220        // size of each packet in the message * the lineCount of them FIXME make 220
  #define MAX_LINES 1000          // Lines used in every sd card file
  #define LED_PIN 4
  #define CAM_CS 5
  #define SD_CS 6
  #define TEMPHUMID_PIN 7U // See SENSOR_PIN as well
  const String file_prefix = "cData"; // + i.csv BE VERY CAREFUL. There are limits on file name length. Must Test. Example (CalicoData1.csv) doesn't work 
  

// Libraries
  #include <IridiumSBD.h>                           // https://github.com/mikalhart/IridiumSBD
  #include <Wire.h>                                 // Included with Arduino
  #include <RTClib.h>                               // https://github.com/adafruit/RTClib
  #include <Multichannel_Gas_GMXXX.h>               // https://github.com/Seeed-Studio/Seeed_Arduino_MultiGas
  #include <HP20x_dev.h>                            // https://github.com/Seeed-Studio/Grove_Barometer_HP20x
  #include "DFRobot_OzoneSensor.h"                  // https://github.com/DFRobot/DFRobot_OzoneSensor
  #include <LTR390.h>                               // https://github.com/levkovigor/LTR390
  #include <SparkFun_u-blox_GNSS_Arduino_Library.h> // https://github.com/sparkfun/SparkFun_u-blox_GNSS_Arduino_Library
  #include <AM2302-Sensor.h>
  #include <SD.h>
  #include <SPI.h>
  #include <ArduCAM.h>

// I2C Intrerface Addresses
  #define I2C_ADDRESS_LTR390 0x53
  #define OZONE_ADDRESS OZONE_ADDRESS_3  // 0x70
  #define GAS_ADDRESS 0x08
  #define TIME_message_SIZE 30 // Increased message size for date/time
  #define GPS_I2C_ADDRESS 0x42 // Default u-blox address
  #define IridiumSerial Serial3 // serial for satelite commmunication

// Sensor objects & Some ports
  RTC_DS3231 rtc;
  SFE_UBLOX_GNSS myGNSS;
  GAS_GMXXX<TwoWire> gas;
  HP20x_dev hp20x;
  DFRobot_OzoneSensor ozone;
  LTR390 ltr390(I2C_ADDRESS_LTR390);
  constexpr unsigned int SENSOR_PIN {TEMPHUMID_PIN}; // Pin 7 for Temperaute/Humidity
  AM2302::AM2302_Sensor am2302{SENSOR_PIN};
  IridiumSBD IridiumModem(IridiumSerial);
  #define COLLECT_NUMBER 20       // Ozone data collection range (1-100)
  ArduCAM myCAM(OV2640, CAM_CS);
  #if !(defined OV2640_MINI_2MP)
  #error Please select the hardware platform and camera module in the ../libraries/ArduCAM/memorysaver.h file
  #endif
  
// DataPacket Struct Info: Not used as class due to tranmission.
  struct __attribute__((packed)) DataPacket {
    /* Info
      55 Byte Sensor Packet Structure (equal to one line of data)
      The __attribyte__((packed)) command makes sure it is exactly 55 by eliminating padding by the compiler.
      Furthermore, there are 3 bytes included in the final message (1 for complete packet lineCount, and 2 for xor checksum)
      Sizes: 
      - uint8_t  : 8  Bits = 1 Byte
      - uint16_t : 16 Bits = 2 Bytes
      - uint32_t : 32 Bits = 4 Bytes
      - float    : 32 Bits = 4 Bytes
    */ 

    // Time (3 Bytes)
    uint8_t second;
    uint8_t minute;
    uint8_t hour;

    // GPS (14 Bytes)
    int32_t latitude;
    int32_t longitude;
    int32_t altitude;
    uint8_t satellites;
    uint8_t fixtype;

    // Sensors (38 Bytes)
    uint32_t NO2;
    uint32_t C2H5OH;
    uint32_t VOC;
    uint32_t CO;
    uint16_t ozone;
    float pressure;
    float uv;
    float lux;
    float temperature;
    float humidity; 
  };
struct DataTotals {
  uint32_t NO2 = 0, C2H5OH = 0, VOC = 0, CO = 0, ozone = 0;
  float pressure = 0, uv = 0, lux = 0, temperature = 0, humidity = 0;
  uint16_t count = 0;

  void add(const DataPacket& p) {
    NO2 += p.NO2; C2H5OH += p.C2H5OH; VOC += p.VOC; CO += p.CO;
    ozone += p.ozone; pressure += p.pressure; uv += p.uv;
    lux += p.lux; temperature += p.temperature; humidity += p.humidity;
    count++;
  }

  DataPacket average(const DataPacket& latestTimeAndGPS) {
    DataPacket avg = latestTimeAndGPS;
    if (count == 0) return avg;
    avg.NO2         = NO2 / count;
    avg.C2H5OH      = C2H5OH / count;
    avg.VOC         = VOC / count;
    avg.CO          = CO / count;
    avg.ozone       = ozone / count;
    avg.pressure    = pressure / count;
    avg.uv          = uv / count;
    avg.lux         = lux / count;
    avg.temperature = temperature / count;
    avg.humidity    = humidity / count;
    return avg;
  }
  void reset() { *this = DataTotals(); }
};
// Time Reference Structure
  struct TimeRef {
    // Used to store rtc time object and millis object. Works with hasSecondspassed() function
    DateTime rtcTime;
    unsigned long sysMillis;
  };

// Function Prototypes
  // (The getSensor group of functions are not here since they are called by collectData())
  bool initRTC();
  bool initGPS();
  bool initGasSensor();
  bool initOzoneSensor();
  bool initBarometer();
  bool initUVSensor();
  bool initIridium();
  bool initCamera();
  bool hasSecondsPassed(TimeRef&, uint32_t);
  DataPacket collectData();
  int sendSBDMessage();
  void setLights(bool on, int startDelay = 0, int endDelay = 0);
  bool writeDataPacket(DataPacket*, size_t); // Overload function for multiple packets
  bool writeDataPacket(DataPacket, File); // Write a packet (row) of time, gps, and sensors to the sd card files.
  void takePicture(); // take and save picture



////////////////////////// DEBUGGING FUNCTIONS PROTOTYPES
bool printDataPacket(DataPacket packet);

// Loop Variables
  bool rtcInitialized = false;
  bool gpsInitialized = false;
  bool callback_status = false;
  bool messageReady = false;
  DataTotals totals;
  DataPacket p;
  DataPacket avg_p;
  DataPacket linesBuffer[LINES_BUFFER_SIZE]; // Buffer containing sensor lines to be written to the sd card (useful for reducing writes)
  DataPacket messageBuffer[PACKETS_PER_MESSAGE]; // message with Message Size
  uint8_t message[MESSAGE_SIZE];
  uint32_t lineCount = 0;
  uint8_t lineIndex = 0;
  uint8_t messagePacketCount = 0; // stores the current index to buffer at
  // Timers are updated internally in hasSecondsPassed
  TimeRef last_sensor;  // time of last sensors read
  TimeRef last_image;   // time of last image capture and save
  TimeRef last_average; // time of last averaged packet
  TimeRef last_message; // time of last sent message

void setup() {
  // Initialize Serial, I2C, SPI
  Serial.begin(MAIN_BAUD); // Set Serial for Printing to 9600 baud as requested
  Wire.begin();
  SPI.begin();
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, LOW); // Turn on
  if(SD.begin(SD_CS)) Serial.println(F("\nSD Initialised Correctly"));

  // Initialize all parts
  rtcInitialized = initRTC();
  gpsInitialized = initGPS();
  initGasSensor();
  initOzoneSensor();
  initBarometer();
  initUVSensor();
  if (initIridium()) Serial.println("Iridium Initialised"); ////////////////////NOTE: CURRENTLY DISABLED FIXME
  initCamera(); 
  Serial.println(F("SENSORS INITIALISED"));
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);   // Start with LEDs ON

  // Setup Initial times.
  last_message.rtcTime = rtc.now();
  last_message.sysMillis = millis();
  last_average = last_message;
  last_sensor = last_message;
  last_image = last_message;
  setLights(false);
}

void loop() {

  setLights(false);
  // Sensor data collection
  if (hasSecondsPassed(last_sensor, SENSOR_INTERVAL) && lineIndex < LINES_BUFFER_SIZE) {
      p = collectData();
      linesBuffer[lineIndex++] = p;
      totals.add(p);
      printDataPacket(p);
      if (lineIndex >= LINES_BUFFER_SIZE) {
        Serial.println("Writing Buffer");
        writeDataPacket(linesBuffer, LINES_BUFFER_SIZE);
        lineIndex = 0;
      }
  }

  // Averaging logic
  if (hasSecondsPassed(last_average, AVERAGING_INTERVAL) && !messageReady) {
      Serial.println("Averaging Data");
      DataPacket avg_p = totals.average(p);
      printDataPacket(avg_p);
      
      // Only add if message is not already full
      messageBuffer[messagePacketCount++] = avg_p;
      if (messagePacketCount == PACKETS_PER_MESSAGE) {
        messageReady = true; // clearly mark message as ready
      }
      printDataPacket(avg_p);
      totals.reset();
  }

//// CURRENTLY DISABLED TRANSMISSION AND PICTURE, ALSO IRIDIUM


  // Transmission logic (simplified and clear)
  if (messageReady && hasSecondsPassed(last_message, MESSAGE_INTERVAL)) { //FIXME change OR to AND, this was to speed up testing
    Serial.println("Sending Messsage");
    messagePacketCount = 0;
    callback_status = true;
    int status = sendSBDMessage();
    callback_status = false;
    if (status == ISBD_SUCCESS) {
      messageReady = false;
      Serial.println(F("\n Message Sent\n"));
    } // If callback cancelled transmission, new message is ready
    else if (status == ISBD_CANCELLED) {
      messageReady = true;
      Serial.println(F("\n\nMessage Cancelled\n\n"));
    }
    else {
      messageReady = false;
      Serial.println(F("\n\nMessage Failed\n\n"));
    }
  } 
    // Image capturing (unchanged)
  if (hasSecondsPassed(last_image, PICTURE_INTERVAL)) {
    Serial.println(F("Taking Image"));
    takePicture();
  }
  setLights(true, 0, 200);
}

// Initialize Real-Time Clock - returns true if successful
bool initRTC() {
  // Try to initialize RTC
  if (!rtc.begin()) {
    return false;
  }
  
  // Uncomment the line below on first upload to set RTC to computer time
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
  // Check if RTC lost power and warn (but don't update automatically)
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    return true;
  }
  return true;
}

// Initialize GPS - updated for u-blox GPS library
bool initGPS() {
  
  // First try the default address (0x42)
  if (myGNSS.begin(Wire, GPS_I2C_ADDRESS)) {
    return true;
  }
  
  // If failed, try alternative address (0x57)
  if (myGNSS.begin(Wire, 0x57)) {
    return true;
  }
  
  // If still failed, try auto-detection
  if (myGNSS.begin(Wire)) {
    return true;
  }
  
  // All attempts failed
  return false;
}

// Initialize Gas Sensor FIXME: Add false conditional (gas.begin has void return)
bool initGasSensor() {
  gas.begin(Wire, GAS_ADDRESS);
}

// Initialise Ozone sensor
bool initOzoneSensor() {
  if (!ozone.begin(OZONE_ADDRESS)) {
    return false;
  } 
  else {
    ozone.setModes(MEASURE_MODE_PASSIVE);
  }
}

// Initialize Barometer FIXME: add false conditional
bool initBarometer() { 
  hp20x.begin();
}

// Initialize UV Sensor
bool initUVSensor() {
  if (!ltr390.init()) return false;
  else {
    // Configure sensor
    ltr390.setMode(LTR390_MODE_ALS);
    ltr390.setGain(LTR390_GAIN_3);
    ltr390.setResolution(LTR390_RESOLUTION_18BIT);
  }
  return true;

}

// Initialize Temperature 
bool initTemperatureSensor() {
    am2302.begin();  // Initialize the AM2302 sensor
    delay(3000);
    bool status = am2302.read();
    int i = 0;
    while (status == false && i < 6) {
      am2302.begin();
      bool status = am2302.read();
      i=i+1;
      delay(500);
    }  // Assuming 0 means a successful read
    if (status == true) {
    return true;
    }
    else {
        return false;
    }
}

bool initIridium() {
  IridiumSerial.begin(19200);  // Iridium SBD baud rate
  if (IridiumModem.begin() == ISBD_SUCCESS) return true;        // Initialize the Iridium IridiumModem
  return false;
}

bool getDateTime(DataPacket &packet) {
  if (!rtcInitialized) {
    return false;
  } 
  DateTime now = rtc.now();
  packet.hour = now.hour();
  packet.minute = now.minute();
  packet.second = now.second();
  return true;
}

bool getGPS(DataPacket &packet) {
  if (gpsInitialized) {
    if (myGNSS.isConnected()) {
      // Get positioning data using u-blox methods
      packet.latitude = myGNSS.getLatitude();  // Save latitude to packet
      packet.longitude = myGNSS.getLongitude();  // Save longitude to packet
      packet.altitude = myGNSS.getAltitude();  // Save altitude to packet
      packet.satellites = myGNSS.getSIV();  // Save satellites lineCount to packet
      packet.fixtype = myGNSS.getFixType();  // Save GPS fix type to packet
      return true;

      // Serial.print(latitude / 10000000.0, 6); // Convert to decimal degrees (usually they are scaled by 1e7)
      // Serial.println(longitude / 10000000.0, 6); // convert to meters from millimeters
    } 
  } 
    return false;
}

bool getGas(DataPacket &packet) {
    packet.NO2 = gas.measure_NO2();       // NO2 concentration in ppm
    packet.C2H5OH = gas.measure_C2H5OH(); // Ethanol concentration in ppm
    packet.VOC = gas.measure_VOC();       // VOC concentration in ppm
    packet.CO = gas.measure_CO();         // CO concentration in ppm
    return true;
}

bool getOzone(DataPacket &packet) {
  int16_t ozoneConcentration = ozone.readOzoneData(COLLECT_NUMBER);
  if (ozoneConcentration <= 0) return false;

    // Valid ozone data
    packet.ozone = ozoneConcentration;
    return true;
}

bool getBaro(DataPacket &packet) {
  float pressure = hp20x.ReadPressure() / 100.0;  // Convert to hPa
  float altitude = hp20x.ReadAltitude() / 100.0;  // Convert to meters
  if (pressure > 0) {  // Assuming a valid reading is nonzero
    packet.pressure = pressure;
    packet.altitude = altitude;
    return true;
  } else {
    return false;
  }
}

// Get UV Sensor Data. (Measures Ambient Light and UVI)
bool getUV(DataPacket &packet) {
    if (!ltr390.newDataAvailable()) {
        return false;
    }
    // Read Ambient Light (Lux)
    ltr390.setGain(LTR390_GAIN_3);
    ltr390.setResolution(LTR390_RESOLUTION_18BIT);
    ltr390.setMode(LTR390_MODE_ALS);
    delay(100);  // Small delay to allow sensor to switch modes

    packet.lux = ltr390.getLux();  // Save Lux to packet

    // Read UV Index
    ltr390.setGain(LTR390_GAIN_18);
    ltr390.setResolution(LTR390_RESOLUTION_20BIT);
    ltr390.setMode(LTR390_MODE_UVS);
    delay(100);  // Small delay to allow sensor to switch modes

    packet.uv = ltr390.getUVI();  // Save UVI to packet
    return true;
}

bool getTempHumidity(DataPacket &packet) {
  bool status = am2302.read();
  delay(500);
  // Assuming 0 means a successful read
  while (status == true) {
    packet.temperature = am2302.get_Temperature();
    packet.humidity= am2302.get_Humidity();
    return true;
  }
  return false;
}

bool hasSecondsPassed(TimeRef& last, uint32_t seconds) {
  if (rtcInitialized) {
    DateTime now = rtc.now();
    TimeSpan elapsed = now - last.rtcTime;
    if (elapsed.totalseconds() >= seconds) {
      last.rtcTime = now;
      last.sysMillis = millis(); // Keep both updated
      return true;
    }
    return false;
  } else {
    unsigned long now = millis();
    if ((now - last.sysMillis) >= seconds * 1000UL) {
      last.sysMillis = now;
      return true;
    }
    return false;
  }
}

// FIXME: Test function to collect data and return a packet
DataPacket collectData() {
 DataPacket packet;
    getDateTime(packet);
    getGPS(packet);
    getUV(packet);
    getBaro(packet);
    getGas(packet);
    getOzone(packet);
    getTempHumidity(packet);
    return packet;
}

// Function to send SBD message using Iridium SBD
int sendSBDMessage() {
    // Copy current messageBuffer contents into message (to not conflict with the messageBuffer being updated by the loop)
    memcpy(message, messageBuffer, MESSAGE_SIZE);

    // Send the message
    return IridiumModem.sendSBDBinary(message, MESSAGE_SIZE);
}

// Background Function that is ran behind the scenes while the rockblock is sending a message. Can terminate whole data sendin by returning false.
// For more info see: https://github.com/mikalhart/IridiumSBD/blob/master/extras/IridiumSBD%20Arduino%20Library%20Documentation.pdf
bool ISBDCallback() {
  if (!callback_status) return true; // This line is so the function does nothing on iridium.begin() command. 
  setLights(false);
  if (hasSecondsPassed(last_sensor, SENSOR_INTERVAL * CALLBACK_INTERVALS_COEFFICIENT) && lineIndex < LINES_BUFFER_SIZE) {
      p = collectData();
      linesBuffer[lineIndex++] = p;
      totals.add(p);
      if (lineIndex == LINES_BUFFER_SIZE - 1) {
        writeDataPacket(linesBuffer, LINES_BUFFER_SIZE);
        lineIndex = 0;
      }
  }

  // Averaging logic
  if (hasSecondsPassed(last_average, AVERAGING_INTERVAL * CALLBACK_INTERVALS_COEFFICIENT)) {
      DataPacket avg_p = totals.average(p);
      
      // Only add if message is not already full
      messageBuffer[messagePacketCount++] = avg_p;
      if (messagePacketCount == PACKETS_PER_MESSAGE) {
        messageReady = true; // clearly mark message as ready
      }
      totals.reset();
  }


  // Image capturing (unchanged)
  if (hasSecondsPassed(last_image, PICTURE_INTERVAL * CALLBACK_INTERVALS_COEFFICIENT)) {
    takePicture();
  }

  setLights(true);
  return true;  // Continue current transmission
}

// Turns LED on/off with optional delays. Assumes active means low LED voltage. Delay taken in millis.
void setLights(bool on, int startDelay = 0, int endDelay = 0) {
  delay(startDelay);
  digitalWrite(LED_PIN, (on ? HIGH : LOW));  // LOW = ON (active-low)
  delay(endDelay);
}

// Save one packet per call to SD
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

// Save Multiple Packets (collected during transmission), Handles File Names
bool writeDataPacket(DataPacket* packets, size_t packetCount) {
  static int fileIndex = 0;
  static String filename;

  // Handle Header for New Files
  if (lineCount == 0 || lineCount >= MAX_LINES) {
    filename = file_prefix + String(fileIndex++) + ".csv"; // DO NOT CHANGE THIS PLEASE: THERE ARE CONSTRAINTS ON FILE NAME LENGTH
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

// NOTE: Debugging and Help Functions
bool printDataPacket(DataPacket packet) {
  Serial.print(packet.second);        Serial.print(",");
  Serial.print(packet.minute);        Serial.print(",");
  Serial.print(packet.hour);          Serial.print(",");
  Serial.print(packet.latitude);      Serial.print(",");
  Serial.print(packet.longitude);     Serial.print(",");
  Serial.print(packet.altitude);      Serial.print(",");
  Serial.print(packet.satellites);    Serial.print(",");
  Serial.print(packet.fixtype);       Serial.print(",");
  Serial.print(packet.NO2);           Serial.print(",");
  Serial.print(packet.C2H5OH);        Serial.print(",");
  Serial.print(packet.VOC);           Serial.print(",");
  Serial.print(packet.CO);            Serial.print(",");
  Serial.print(packet.ozone);         Serial.print(",");
  Serial.print(packet.pressure);      Serial.print(",");
  Serial.print(packet.uv);            Serial.print(",");
  Serial.print(packet.lux);           Serial.print(",");
  Serial.print(packet.temperature);   Serial.print(",");
  Serial.println(packet.humidity);
  return true;
}

// FIXME: Change to use time instead of numbers
void takePicture(){
  char str[8];
  byte buf[256];
  static int i = 0;
  static int k = 0;
  uint8_t temp = 0,temp_last=0;
  uint32_t length = 0;
  bool is_header = false;
  File outFile;
  //Flush the FIFO
  myCAM.flush_fifo();
  //Clear the capture done flag
  myCAM.clear_fifo_flag();
  //Start capture
  myCAM.start_capture();
  Serial.println(F("start Capture"));
  while(!myCAM.get_bit(ARDUCHIP_TRIG , CAP_DONE_MASK));
  Serial.println(F("Capture Done."));  
  length = myCAM.read_fifo_length();
  Serial.print(F("The fifo length is :"));
  Serial.println(length, DEC);
  //Construct a file name using time
  DateTime now = rtc.now();
  String imgname = "p" + String(now.hour()) + String(now.minute()) + ".jpg";
  //Open the new file
  outFile = SD.open((String("Pictures/") + imgname).c_str(), O_WRITE | O_CREAT | O_TRUNC);
  if(!outFile){
    Serial.println(F("File open failed"));
    return;
  }
  myCAM.CS_LOW();
  myCAM.set_fifo_burst();
  while ( length-- )
  {
    temp_last = temp;
    temp =  SPI.transfer(0x00);
    //Read JPEG data from FIFO
    if ( (temp == 0xD9) && (temp_last == 0xFF) ) //If find the end ,break while,
    {
      buf[i++] = temp;  //save the last  0XD9     
      //Write the remain bytes in the buffer
      myCAM.CS_HIGH();
      outFile.write(buf, i);    
      //Close the file
      outFile.close();
      Serial.println(F("Image save OK."));
      is_header = false;
      i = 0;
    }  
    if (is_header == true)
    { 
      //Write image data to buffer if not full
      if (i < 256)
      buf[i++] = temp;
      else
      {
        //Write 256 bytes image data to file
        myCAM.CS_HIGH();
        outFile.write(buf, 256);
        i = 0;
        buf[i++] = temp;
        myCAM.CS_LOW();
        myCAM.set_fifo_burst();
      }        
    }
    else if ((temp == 0xD8) & (temp_last == 0xFF))
    {
      is_header = true;
      buf[i++] = temp_last;
      buf[i++] = temp;   
    } 
  } 
}

bool initCamera() {
  uint8_t vid, pid;
  uint8_t temp;
  //set the CS as an output:
  pinMode(CAM_CS,OUTPUT);
  digitalWrite(CAM_CS, HIGH);
  //Reset the CPLD
  myCAM.write_reg(0x07, 0x80);
  delay(100);
  myCAM.write_reg(0x07, 0x00);
  delay(100);

  //Check if the ArduCAM SPI bus is OK
  myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
  temp = myCAM.read_reg(ARDUCHIP_TEST1);
  
  if (temp != 0x55){
    Serial.println(F("SPI interface Error!"));
    return false;
  }

    //Check if the camera module type is OV2640
    myCAM.wrSensorReg8_8(0xff, 0x01);
    myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
    myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
    if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 ))){
      Serial.println(F("Can't find OV2640 module!"));
      return false;
    }

  myCAM.set_format(JPEG);
  myCAM.InitCAM();
  myCAM.OV2640_set_JPEG_size(OV2640_1600x1200);
}


