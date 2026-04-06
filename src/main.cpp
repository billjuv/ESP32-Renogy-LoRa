#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <ModbusMaster.h>

// ---- LoRa pins ----
#define LORA_CS     5
#define LORA_RST    14
#define LORA_DIO0   2

// ---- Renogy UART2 pins ----
#define RXD2        16
#define TXD2        17
#define RENOGY_ADDR 0xFF

// ---- Device ID - change to match your controller ----
//#define DEVICE_ID   "renogy_wonderer"
 #define DEVICE_ID "renogy_rover20"

// ---- How often to send (milliseconds) ----
#define SEND_INTERVAL 15000

ModbusMaster node;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  node.begin(RENOGY_ADDR, Serial2);

  Serial.println("Starting Renogy LoRa transmitter...");

  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(915E6)) {
    Serial.println("LoRa init failed! Check wiring.");
    while (true);
  }
  LoRa.setSyncWord(0x12);
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  Serial.println("LoRa ready.");
}

void loop() {
  static unsigned long lastSend = 0;

  if (millis() - lastSend >= SEND_INTERVAL) {
    lastSend = millis();

    uint8_t result = node.readHoldingRegisters(0x0100, 17);

    if (result == node.ku8MBSuccess) {

      int   soc        = node.getResponseBuffer(0x00);
      float battV      = node.getResponseBuffer(0x01) / 10.0;
      float battA      = node.getResponseBuffer(0x02) / 100.0;

      uint16_t tempRaw = node.getResponseBuffer(0x03);
      int battTemp     = tempRaw & 0x7F;
      if ((tempRaw & 0x0080) >> 7)  battTemp = -battTemp;
      int ctrlTemp     = (tempRaw & 0x7F00) >> 8;
      if ((tempRaw & 0x8000) >> 15) ctrlTemp = -ctrlTemp;

      float solarV     = node.getResponseBuffer(0x07) / 10.0;
      float solarA     = node.getResponseBuffer(0x08) / 100.0;
      int   solarW     = node.getResponseBuffer(0x09);

      float battVMin   = node.getResponseBuffer(0x0B) / 10.0;
      float battVMax   = node.getResponseBuffer(0x0C) / 10.0;
      int   solarWMax  = node.getResponseBuffer(0x0F);
      int   solarWMin  = node.getResponseBuffer(0x10);

      String payload = "{";
      payload += "\"value\":\""  + String(DEVICE_ID) + "\",";
      payload += "\"id\":\""     + String(DEVICE_ID) + "\",";
      payload += "\"batt_v\":"   + String(battV,   1) + ",";
      payload += "\"batt_a\":"   + String(battA,   2) + ",";
      payload += "\"soc\":"      + String(soc)         + ",";
      payload += "\"solar_v\":"  + String(solarV,  1) + ",";
      payload += "\"solar_a\":"  + String(solarA,  2) + ",";
      payload += "\"solar_w\":"  + String(solarW)      + ",";
      payload += "\"batt_t\":"   + String(battTemp)    + ",";
      payload += "\"ctrl_t\":"   + String(ctrlTemp)    + ",";
      payload += "\"solar_w_max\":" + String(solarWMax)   + ",";
      payload += "\"solar_w_min\":" + String(solarWMin)   + ",";
      payload += "\"batt_v_max\":" + String(battVMax, 1)  + ",";
      payload += "\"batt_v_min\":" + String(battVMin, 1);
      payload += "}";

      Serial.println("Sending: " + payload);

      LoRa.beginPacket();
      LoRa.print(payload);
      LoRa.endPacket();

      Serial.println("Sent.");

    } else {
      Serial.print("Modbus error: 0x");
      Serial.println(result, HEX);
    }
  }
}
