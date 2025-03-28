#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

#define LORAWAN_APP_PORT 1
#define LORAWAN_SEND_INTERVAL_MINUTES 1  // Change as needed
#define LORAWAN_SEND_INTERVAL (LORAWAN_SEND_INTERVAL_MINUTES * 60 * 1000)

unsigned long lastSendTime = 0;
String receivedData = ""; // Store received serial data
byte payloadData[8]; // Declare the payload data array

// OTAA credentials (Replace with actual values)
static u1_t APPEUI[8] = { 0x60, 0x81, 0xF9, 0xA1, 0x8A, 0xEB, 0x66, 0xCB }; 
static u1_t DEVEUI[8] = { 0x60, 0x81, 0xF9, 0x2B, 0x8A, 0x3C, 0xA8, 0xFB };
static u1_t APPKEY[16] = { 0xE2, 0x08, 0x00, 0x8B, 0x95, 0xDE, 0x7C, 0x72, 0xFD, 0xEA, 0x9B, 0x3C, 0x25, 0x53, 0x6F, 0x21 };

// Define LoRa pins
const lmic_pinmap lmic_pins = {
    .nss = 5,  
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 14,  
    .dio = {2, 3, LMIC_UNUSED_PIN} 
};

void os_getDevKey(u1_t* buf) { memcpy(buf, APPKEY, 16); }
void os_getArtEui(u1_t* buf) { memcpy(buf, APPEUI, 8); }
void os_getDevEui(u1_t* buf) { memcpy(buf, DEVEUI, 8); }

void setup() {
    Serial.begin(115200);  // Use default serial for debugging
    // Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);  // External sensor communication if using Serial2
    
    os_init();
    LMIC_reset();
    Serial.println("Joining LoRaWAN Network...");
    LMIC_startJoining();
}

void loop() {
    os_runloop_once();

    // Read data from the serial port (Serial monitor)
    while (Serial.available()) {
        char c = Serial.read();
        receivedData += c;
        if (c == '\n') { // End of data line
            processSerialData(receivedData);
            receivedData = ""; // Clear buffer
        }
    }

    // Send data at set intervals
    if (millis() - lastSendTime >= LORAWAN_SEND_INTERVAL) {
        lastSendTime = millis();
        sendLoRaPayload();
    }
}

// Process received serial data
void processSerialData(String data) {
    Serial.print("Received from Sensor: ");
    Serial.println(data);

    // Example format: "0.7,1.0,197,24.8,85,2980"
    float wind_avg, wind_max, temperature;
    int wind_dir, humidity, battery_mV;

    sscanf(data.c_str(), "%f,%f,%d,%f,%d,%d", &wind_avg, &wind_max, &wind_dir, &temperature, &humidity, &battery_mV);

    // Convert to LoRa payload format
    uint8_t wind_avg_knots = round(wind_avg * 1.94384 * 10);
    uint8_t wind_max_knots = round(wind_max * 1.94384 * 10);
    uint8_t wind_dir_scaled = wind_dir / 2;
    uint8_t temp_C_scaled = (uint8_t)(temperature + 40);
    uint8_t hum_scaled = (uint8_t)humidity;
    uint16_t battery_mV_scaled = battery_mV;
    
    // Store converted values for transmission in payloadData
    payloadData[0] = 0x03; // Data type identifier
    payloadData[1] = wind_avg_knots;
    payloadData[2] = wind_max_knots;
    payloadData[3] = wind_dir_scaled;
    payloadData[4] = temp_C_scaled;
    payloadData[5] = hum_scaled;
    payloadData[6] = (battery_mV_scaled >> 8) & 0xFF;
    payloadData[7] = battery_mV_scaled & 0xFF;
}

// Prepare and send LoRa payload
void sendLoRaPayload() {
    Serial.println("Sending LoRaWAN Payload...");
    LMIC_setTxData2(LORAWAN_APP_PORT, payloadData, sizeof(payloadData), 0);

    Serial.print("Wind Avg (knots): "); Serial.println(payloadData[1] / 10.0);
    Serial.print("Wind Max (knots): "); Serial.println(payloadData[2] / 10.0);
    Serial.print("Wind Direction: "); Serial.println(payloadData[3] * 2);
    Serial.print("Temperature (C): "); Serial.println(payloadData[4] - 40);
    Serial.print("Humidity (%): "); Serial.println(payloadData[5]);
    Serial.print("Battery (mV): "); Serial.println((payloadData[6] << 8) | payloadData[7]);
}
