#include <Arduino.h>

// Define the DataPacket struct
struct __attribute__((packed)) DataPacket {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;

    uint32_t latitude;
    uint32_t longitude;
    uint32_t altitude;
    uint8_t satellites;
    uint8_t fixtype;

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

// Function to decode a binary data buffer into a DataPacket struct. Given a pointer to the packet 
void decodeDataPacket(const uint8_t *buffer, DataPacket &packet) {
    memcpy(&packet, buffer, sizeof(DataPacket));
}

// Function to print the decoded packet for verification
void printDataPacket(const DataPacket &packet) {
    Serial.println("=== Decoded Data Packet ===");
    
    Serial.print("Time: ");
    Serial.print(packet.hour);
    Serial.print(":");
    Serial.print(packet.minute);
    Serial.print(":");
    Serial.println(packet.second);
    
    Serial.print("GPS -> Latitude: ");
    Serial.print(packet.latitude);
    Serial.print(", Longitude: ");
    Serial.print(packet.longitude);
    Serial.print(", Altitude: ");
    Serial.print(packet.altitude);
    Serial.print(", Satellites: ");
    Serial.print(packet.satellites);
    Serial.print(", FixType: ");
    Serial.println(packet.fixtype);

    Serial.print("Gas -> NO2: ");
    Serial.print(packet.NO2);
    Serial.print(", C2H5OH: ");
    Serial.print(packet.C2H5OH);
    Serial.print(", VOC: ");
    Serial.print(packet.VOC);
    Serial.print(", CO: ");
    Serial.println(packet.CO);

    Serial.print("Ozone: ");
    Serial.print(packet.ozone);
    Serial.println(" ppb");

    Serial.print("Pressure: ");
    Serial.print(packet.pressure);
    Serial.println(" hPa");

    Serial.print("UV Index: ");
    Serial.println(packet.uv);

    Serial.print("Lux: ");
    Serial.print(packet.lux);
    Serial.println(" lx");

    Serial.print("Temperature: ");
    Serial.print(packet.temperature);
    Serial.println(" C");

    Serial.print("Humidity: ");
    Serial.print(packet.humidity);
    Serial.println(" %");

    Serial.println("=================================");
}

void setup() {
    Serial.begin(9600);
    delay(1000);  // Small delay to ensure serial is ready

    // Example binary data (Simulated received data) with packet count as the first byte
    uint8_t exampleData[] = {
        2,  // Packet count (2 packets in the buffer)
        
        // First DataPacket
        10, 30, 12,  // Time: 12:30:10
        0x78, 0x56, 0x34, 0x12,  // Latitude
        0x21, 0x43, 0x65, 0x87,  // Longitude
        0x12, 0x34, 0x56, 0x78,  // Altitude
        5, 1,  // Satellites: 5, FixType: 1
        0x11, 0x22, 0x33, 0x44,  // NO2
        0x55, 0x66, 0x77, 0x88,  // C2H5OH
        0x99, 0xAA, 0xBB, 0xCC,  // VOC
        0xDD, 0xEE, 0xFF, 0x00,  // CO
        0x01, 0x02,  // Ozone
        0x00, 0x00, 0xA0, 0x41,  // Pressure (20.0 hPa)
        0x00, 0x00, 0x80, 0x3F,  // UV (1.0)
        0x00, 0x00, 0xC8, 0x42,  // Lux (100.0 lx)
        0x00, 0x00, 0x48, 0x42,  // Temperature (50.0 C)
        0x00, 0x00, 0x70, 0x42,  // Humidity (60.0 %)

        // Second DataPacket
        20, 40, 2,  // Time: 02:40:20
        0x87, 0x65, 0x43, 0x21,  // Latitude
        0x76, 0x54, 0x32, 0x10,  // Longitude
        0x34, 0x56, 0x78, 0x90,  // Altitude
        6, 2,  // Satellites: 6, FixType: 2
        0x22, 0x33, 0x44, 0x55,  // NO2
        0x66, 0x77, 0x88, 0x99,  // C2H5OH
        0xAA, 0xBB, 0xCC, 0xDD,  // VOC
        0xEE, 0xFF, 0x00, 0x11,  // CO
        0x02, 0x03,  // Ozone
        0x00, 0x00, 0xB0, 0x41,  // Pressure (22.0 hPa)
        0x00, 0x00, 0x90, 0x3F,  // UV (1.5)
        0x00, 0x00, 0xD0, 0x42,  // Lux (120.0 lx)
        0x00, 0x00, 0x58, 0x42,  // Temperature (56.0 C)
        0x00, 0x00, 0x80, 0x42   // Humidity (65.0 %)
    };

    // Get packet count from the first byte
    uint8_t packetCount = exampleData[0];
    Serial.print("Packet Count: ");
    Serial.println(packetCount);

    // Iterate through the buffer and decode each packet
    for (uint8_t i = 0; i < packetCount; i++) {
        uint8_t *packetData = &exampleData[1 + i * sizeof(DataPacket)];  // Offset by 1 for packet count byte (pointer to packet)
        DataPacket decodedPacket;
        decodeDataPacket(packetData, decodedPacket); // decode packet from pointer and store to print
        printDataPacket(decodedPacket);
    }
}

void loop() {
    // Nothing here, runs once in setup()
}
