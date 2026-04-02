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
//#define RENOGY_ADDR 0xFF // Wonderer 10A
#define RENOGY_ADDR 0xFF //Rover 20A
// ---- How often to send (milliseconds) ----
#define SEND_INTERVAL 60000

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

    uint8_t result = node.readHoldingRegisters(0x0100, 12);

    if (result == node.ku8MBSuccess) {

      float battV    = node.getResponseBuffer(0x01) / 10.0;
      float battA    = node.getResponseBuffer(0x02) / 100.0;
      int   soc      = node.getResponseBuffer(0x00);
      float solarV   = node.getResponseBuffer(0x07) / 10.0;
      float solarA   = node.getResponseBuffer(0x08) / 100.0;
      int   solarW   = node.getResponseBuffer(0x09);
      uint16_t tempRaw   = node.getResponseBuffer(0x03);
      int ctrlTemp  = (tempRaw & 0x7F00) >> 8;
      if ((tempRaw & 0x8000) >> 15) ctrlTemp = -ctrlTemp;
      int battTemp  = tempRaw & 0x7F;
      if ((tempRaw & 0x0080) >> 7)  battTemp = -battTemp;

     String payload = "{";
      payload += "\"value\":\"renogy_rover20\",";
      payload += "\"id\":\"renogy_rover20\",";
      payload += "\"batt_v\":"  + String(battV,  1) + ",";
      payload += "\"batt_a\":"  + String(battA,  2) + ",";
      payload += "\"soc\":"     + String(soc)        + ",";
      payload += "\"solar_v\":" + String(solarV, 1) + ",";
      payload += "\"solar_a\":" + String(solarA, 2) + ",";
      payload += "\"solar_w\":" + String(solarW)     + ",";
      payload += "\"batt_t\":"  + String(battTemp)  + ",";
      payload += "\"ctrl_t\":"  + String(ctrlTemp);
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