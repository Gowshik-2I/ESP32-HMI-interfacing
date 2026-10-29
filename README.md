# ESP32 Modbus RTU — Industrial HMI Communication

> A bare-metal implementation of the Modbus RTU protocol on ESP32, built entirely from scratch without any external libraries. The ESP32 acts as a Modbus RTU slave, communicating with a **Samkoon EA-043A industrial HMI** over RS485 using a MAX485 transceiver module. Live analog data from an LDR (light dependent resistor) sensor is read and displayed in real time on the HMI screen.

---

## Why This Project Exists

Most ESP32 Modbus tutorials tell you to install a library and call a function. This project does the opposite.

Every byte of every Modbus frame is constructed manually. The CRC16 checksum is calculated bit by bit. The RS485 bus direction is switched by hand. The inter-frame gap is timed using milliseconds.

The goal was to understand industrial communication at the lowest practical level — the way it actually works inside those libraries — before ever using one.

---

## Demo


https://github.com/user-attachments/assets/cecfe923-aafe-4176-a98a-9f41a2506b3c



```
LDR Sensor (analog light reading)
        ↓
      ESP32
        ↓  [Modbus RTU over RS485]
    MAX485 Module
        ↓  [A+/B- differential signal]
  DB9 Female Connector
        ↓
 Samkoon EA-043A HMI
        ↓
  Live value displayed on screen
```

---

## Hardware Used

| Component | Description |
|---|---|
| ESP32 DevKit | Microcontroller, acts as Modbus RTU slave |
| Samkoon EA-043A | 4.3" Industrial HMI, acts as Modbus RTU master |
| MAX485 Module | RS485 transceiver for differential signaling |
| LDR Sensor | Light dependent resistor for analog light intensity reading |
| DB9 Female Connector | Hand-soldered, connects to HMI COM port |
| Jumper Wires | For all connections |

---

## Wiring

### HMI DB9 to MAX485

| HMI DB9 Pin | Signal | MAX485 Pin |
|---|---|---|
| Pin 9 | A+ (Data+) | A |
| Pin 8 | B− (Data−) | B |
| Pin 5 | GND | GND |

> The DB9 connector was hand-soldered with jumper wires. The HMI COM port supports RS485 on pins 8 and 9 — these are non-standard RS232 pins repurposed by Samkoon for RS485.

### MAX485 to ESP32

| MAX485 Pin | ESP32 Pin | Description |
|---|---|---|
| RO | GPIO 16 (RX) | Receive data |
| RE | GPIO 4 | Receiver enable (active LOW) |
| DE | GPIO 4 | Driver enable (active HIGH) |
| DI | GPIO 17 (TX) | Transmit data |
| VCC | 3.3V | Power |
| GND | GND | Ground |

> RE and DE are tied together to a single GPIO pin for half-duplex direction control.

### LDR to ESP32

| LDR Circuit | ESP32 Pin | Description |
|---|---|---|
| LDR + 10kΩ voltage divider output | GPIO 33 (ADC) | Analog light reading |

> Note: The pin is defined as `IR` in code but the connected sensor is an LDR. Reading is inverted using `4095 - analogRead(IR)` so that higher light intensity = higher displayed value on HMI.

---

## How Modbus RTU Works (in this project)

Modbus RTU is a serial communication protocol where one **master** (the HMI) sends requests and one or more **slaves** (the ESP32) respond.

### Request frame sent by HMI (FC03 — Read Holding Registers)

```
| Slave ID | Function Code | Start Addr High | Start Addr Low | Count High | Count Low | CRC Low | CRC High |
|   0x01   |     0x03      |      0x00       |      0x00      |    0x00    |    0x01   |   XX    |    XX    |
```

### Response frame sent by ESP32

```
| Slave ID | Function Code | Byte Count | Data High | Data Low | CRC Low | CRC High |
|   0x01   |     0x03      |    0x02    |    HH     |    LL    |   XX    |    XX    |
```

Where `HH LL` is the 16-bit LDR analog reading (0–4095 from ESP32 ADC).

### CRC16 Algorithm

The CRC16 (polynomial 0xA001) is calculated manually over all bytes except the last two:

```cpp
uint16_t CRC_Calc(uint8_t arr[], uint8_t leng) {
    uint16_t calculated = 0xFFFF;
    for (int i = 0; i < leng - 2; i++) {
        calculated = arr[i] ^ calculated;
        for (int j = 0; j < 8; j++) {
            if (calculated & 0x0001) {
                calculated = calculated >> 1;
                calculated = calculated ^ 0xA001;
            } else {
                calculated = calculated >> 1;
            }
        }
    }
    return calculated;
}
```

### Inter-frame Gap Detection

Modbus RTU marks the end of a frame using silence on the bus (3.5 character times). At 9600 baud this is ~3.65ms. This project uses a 6ms timeout for reliability:

```cpp
if (millis() - last_recp >= 6 && count != 0) {
    processFrame(arr, count);
    count = 0;
}
```

---

## Software Setup

### HMI Configuration (SATool 6.0)

1. Open SATool and create a new project for EA-043A
2. Add a new link:
   - Link Type: `Direct Connection (Serial Port)`
   - Device/Server: `Modbus` / `Modbus RTU Master`
   - Link Port: `COM1`
3. Serial settings:
   - Baud Rate: `9600`
   - Data Bits: `8`
   - Parity: `None`
   - Stop Bits: `1`
   - Station No: `1`
4. Add a **Numeric Display** element on your screen
5. Set its address to Holding Register `40001` (register address `0x0000`, slave ID `1`)
6. Download the project to HMI via USB

### ESP32 (Arduino IDE)

No libraries required. Just open the `.ino` file and flash it to your ESP32.

```
Board: ESP32 Dev Module
Upload Speed: 921600
CPU Frequency: 240MHz
```

---

## Project Structure

```
ESP32-Modbus-RTU-HMI/
│
└── modbus_ldr.ino       # Single file — complete implementation
```

Everything is in one file intentionally — to keep it readable and easy to follow for anyone learning Modbus RTU from scratch.

---

## Key Concepts Demonstrated

- **Modbus RTU protocol** — frame structure, function codes, register addressing
- **CRC16 checksum** — bit-by-bit implementation using polynomial 0xA001
- **RS485 half-duplex communication** — DE/RE pin control for bus direction switching
- **UART on ESP32** — using Serial2 with custom RX/TX pins
- **ADC reading** — converting analog LDR resistance to a 12-bit digital value
- **Inter-frame gap timing** — detecting end of Modbus frame using silence duration

---

## Known Limitations & Future Improvements

- [ ] Fix operator precedence bug in `CRCcheck()` — `((crc>>8) & 0xFF)` needs extra parentheses
- [ ] Support multiple registers (currently always responds to any register request with LDR value)
- [ ] Add FC06 (Write Single Register) to allow HMI to control a relay
- [ ] Add HC-SR04 ultrasonic sensor for distance/level monitoring
- [ ] Add exception responses for invalid function codes or register addresses

---

## What I Learned

This project was built as part of a deep-dive into industrial embedded systems. Starting from zero knowledge of UART, RS485, and Modbus, the learning path covered:

- Hexadecimal and binary number systems
- Bitwise operations in C++
- UART and serial communication fundamentals
- RS485 differential signaling and the MAX485 transceiver
- Modbus RTU protocol specification — frame structure, timing, CRC

The result is a working industrial communication system built entirely from first principles.

---

## References

- [Modbus Application Protocol Specification V1.1b3](https://modbus.org/docs/Modbus_Application_Protocol_V1_1b3.pdf)
- [Samkoon EA-043A HMI Documentation](https://www.samkoon.store)
- [MAX485 Datasheet](https://www.analog.com/en/products/max485.html)

---

## License

MIT License — free to use, modify, and distribute.
