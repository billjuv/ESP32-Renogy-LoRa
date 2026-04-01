# ESP32-Renogy-LoRa

A standalone LoRa transmitter that reads data from a Renogy solar charge controller via RS232 Modbus and transmits it wirelessly to an OpenMQTTGateway (OMG) LoRa gateway. Built for remote monitoring where WiFi is unavailable or impractical.

Tested with:
- Renogy Wonderer 10A PWM (RNG-CTRL-WND10)
- Renogy Rover 20A MPPT

Should work with any Renogy charge controller that has an RS232 RJ12 port.

---

## How It Works

The ESP32 polls the charge controller every 60 seconds via Modbus RTU over RS232 (through a MAX3232 level converter), builds a JSON payload, and transmits it via LoRa. The OMG gateway receives the packet and publishes it to MQTT, where it can be picked up by Node-RED, InfluxDB, and Grafana.

```
Renogy controller → RJ12/RS232 → MAX3232 → ESP32 → RFM95W → LoRa RF → OMG gateway → MQTT
```

---

## Hardware

- ESP32 DevKit1
- Adafruit RFM95W LoRa transceiver (915MHz)
- MAX3232 TTL/RS232 converter board
- RJ12 6-wire cable

---

## Wiring

### RJ12 to MAX3232 (RS232 side)
Pins counted right-to-left with contacts facing you.

| RJ12 Pin | Signal | MAX3232 RS232 side |
|----------|--------|--------------------|
| Pin 1 | Controller TX | RXD |
| Pin 2 | Controller RX | TXD |
| Pin 3 | GND | GND |

> ⚠️ Never connect RS232 lines directly to the ESP32 — the voltage levels will damage it. Always use the MAX3232 converter.

### MAX3232 (TTL side) to ESP32

| MAX3232 TTL | ESP32 GPIO |
|-------------|------------|
| TX | GPIO 16 (RX2) |
| RX | GPIO 17 (TX2) |
| VCC | 3.3V |
| GND | GND |

### RFM95W to ESP32

| RFM95W | ESP32 GPIO |
|--------|------------|
| MOSI | GPIO 23 |
| MISO | GPIO 19 |
| SCK | GPIO 18 |
| CS | GPIO 5 |
| RST | GPIO 14 |
| G0 (DIO0) | GPIO 2 |
| VIN | 3.3V |
| GND | GND |

> Note: The pin labeled **G0** on the Adafruit RFM95W breakout is DIO0. EN can be left unconnected.

---

## Power

Both controllers have been tested using their built-in USB-A port to power the ESP32:

| Controller | USB Voltage | Notes |
|------------|-------------|-------|
| Wonderer 10A | ~11.3V | Use a USB cable directly to ESP32 USB port |
| Rover 20A | ~15.1V | Use a USB cable directly to ESP32 USB port |

> ⚠️ The RJ12 port on the Rover 20A supplies ~15V on pins 4-6 — do NOT use this to power the ESP32 directly without a step-down converter.

The USB-A ports on both controllers appear to be always active when the battery has charge, making them a convenient and clean power source.

---

## Modbus Settings

Both controllers tested at:
- **Baud rate:** 9600
- **Slave address:** 0xFF

> Note: The default documented slave address for Renogy controllers is 0x01, but both units tested here responded to 0xFF. If you get no response, use the included scan utility to find your controller's address.

---

## LoRa Settings

Matched to an existing OpenMQTTGateway LoRa gateway:

| Setting | Value |
|---------|-------|
| Frequency | 915 MHz |
| Spreading Factor | SF7 |
| Bandwidth | 125 kHz |
| Sync Word | 0x12 |

---

## MQTT Output

The transmitter publishes to your OMG gateway, which forwards to MQTT. The `"value"` field in the payload causes OMG to create a dedicated subtopic automatically:

**Topic:**
```
MushLoRa/OMG_LORA_MOAPA/LORAtoMQTT/renogy_wonderer
```

**Payload example:**
```json
{
  "value": "renogy_wonderer",
  "id": "renogy_wonderer",
  "batt_v": 13.2,
  "batt_a": 0.09,
  "soc": 100,
  "solar_v": 13.6,
  "solar_a": 0.07,
  "solar_w": 1,
  "batt_t": 0,
  "ctrl_t": 25,
  "rssi": -79,
  "snr": 9.75,
  "pferror": 226,
  "packetSize": 153
}
```

> Note: `batt_t` will always read 0 on the Wonderer 10A as it has no external battery temperature sensor connection. The Rover 20A returns a real value.

---

## PlatformIO Setup

**platformio.ini:**
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps =
    sandeepmistry/LoRa @ ^0.8.0
    4-20ma/ModbusMaster @ ^2.0.1
```

Set the device ID in `main.cpp` to match your controller:
```cpp
// For Wonderer 10A:
#define DEVICE_ID "renogy_wonderer"

// For Rover 20A:
#define DEVICE_ID "renogy_rover20"
```

---

## Scan Utility

If your controller doesn't respond, use the included scan utility (`tools/scan.cpp`) to find the correct baud rate and Modbus address. It tries the most common combinations first before falling back to a full 0x00–0xFF sweep.

---

## Node-RED / InfluxDB / Grafana

> 🔧 To be documented — Node-RED flow to subscribe to the MQTT topic, parse the payload, and write fields to InfluxDB for display in a Grafana dashboard.

---

## Notes

- The Wonderer 10A responds to Modbus at 2400 baud in some configurations and 9600 in others — if 9600 fails, try 2400.
- Signal range tested at -79 RSSI at the far end of a residential yard using a small coil antenna oriented horizontally. A vertical antenna will improve this.
- The 60-second poll interval is conservative and well within LoRa duty cycle limits.

---

## Related Projects

- [wrybread/ESP32ArduinoRenogy](https://github.com/wrybread/ESP32ArduinoRenogy) — inspiration for the Modbus register approach
- [OpenMQTTGateway](https://github.com/1technophile/OpenMQTTGateway) — the LoRa gateway firmware
