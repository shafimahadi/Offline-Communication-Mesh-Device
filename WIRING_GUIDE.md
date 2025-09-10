# Hardware Wiring Guide

This guide provides detailed wiring instructions for assembling the ESP32 Mesh Communicator. Follow these connections carefully to ensure proper operation.

## 🔧 Tools Required

- Soldering iron and solder
- Wire strippers
- Multimeter (for testing connections)
- Breadboard or perfboard
- Jumper wires (male-to-male, male-to-female)
- Heat shrink tubing (optional)

## ⚡ Power Supply Requirements

**Important**: Verify voltage requirements before connecting power!

- **ESP32**: 5V via USB or 3.3V on VIN pin
- **LoRa SX1278**: 3.3V (NOT 5V - will damage module!)
- **GPS NEO-M8N**: 3.3V-5V (3.3V recommended)
- **OLED SH1106**: 3.3V-5V
- **SD Card Module**: 3.3V-5V
- **Keypad**: 3.3V logic level

## 📋 Component Wiring

### ESP32 Development Board
```
ESP32 Pin Layout (30-pin version):
                    ┌─────────────────┐
                3V3 │1              30│ VIN
                 EN │2              29│ GND
              GPIO36 │3              28│ GPIO13
              GPIO39 │4              27│ GPIO12
              GPIO34 │5              26│ GPIO14
              GPIO35 │6              25│ GPIO27
              GPIO32 │7              24│ GPIO26
              GPIO33 │8              23│ GPIO25
              GPIO25 │9              22│ GPIO33
              GPIO26 │10             21│ GPIO32
              GPIO27 │11             20│ GPIO35
              GPIO14 │12             19│ GPIO34
              GPIO12 │13             18│ GPIO5
                 GND │14             17│ GPIO18
              GPIO13 │15             16│ GPIO19
                    └─────────────────┘
```

### 1. OLED Display (SH1106) - I2C Connection

**Module Pins:**
- VCC → ESP32 3.3V
- GND → ESP32 GND  
- SDA → ESP32 GPIO21
- SCL → ESP32 GPIO22

```
ESP32          SH1106 OLED
┌─────┐        ┌───────────┐
│ 3V3 │────────│ VCC       │
│ GND │────────│ GND       │
│ G21 │────────│ SDA       │
│ G22 │────────│ SCL       │
└─────┘        └───────────┘
```

**Notes:**
- Some OLED modules have VCC/GND pins swapped - check your module!
- Default I2C address is usually 0x3C
- Pull-up resistors (4.7kΩ) may be required on SDA/SCL if not built-in

### 2. GPS Module (NEO-M8N) - UART Connection

**Module Pins:**
- VCC → ESP32 3.3V
- GND → ESP32 GND
- RX → ESP32 GPIO16 (TX2)
- TX → ESP32 GPIO17 (RX2)

```
ESP32          NEO-M8N GPS
┌─────┐        ┌───────────┐
│ 3V3 │────────│ VCC       │
│ GND │────────│ GND       │
│ G16 │────────│ RX        │
│ G17 │────────│ TX        │
└─────┘        └───────────┘
```

**Notes:**
- Connect GPS antenna for best reception
- Initial GPS fix may take 2-5 minutes outdoors
- Some modules have additional pins (PPS, RESET) - leave unconnected

### 3. LoRa Module (SX1278) - SPI Connection

**Module Pins:**
- VCC → ESP32 3.3V ⚠️ **CRITICAL: 3.3V ONLY!**
- GND → ESP32 GND
- MOSI → ESP32 GPIO23
- MISO → ESP32 GPIO19
- SCK → ESP32 GPIO18
- NSS/CS → ESP32 GPIO5
- RST → ESP32 GPIO15
- DIO0 → ESP32 GPIO4

```
ESP32          SX1278 LoRa
┌─────┐        ┌───────────┐
│ 3V3 │────────│ VCC       │ ⚠️ 3.3V ONLY!
│ GND │────────│ GND       │
│ G23 │────────│ MOSI      │
│ G19 │────────│ MISO      │
│ G18 │────────│ SCK       │
│ G5  │────────│ NSS/CS    │
│ G15 │────────│ RST       │
│ G4  │────────│ DIO0      │
└─────┘        └───────────┘
```

**Critical Notes:**
- ⚠️ **SX1278 modules are 3.3V ONLY** - 5V will permanently damage them!
- Connect appropriate antenna (433MHz or 169MHz)
- DIO1, DIO2 pins can be left unconnected for basic operation
- Some modules have different pin layouts - verify before connecting

### 4. SD Card Module - SPI Connection (Shared with LoRa)

**Module Pins:**
- VCC → ESP32 3.3V or 5V (check module specs)
- GND → ESP32 GND
- MOSI → ESP32 GPIO23 (shared with LoRa)
- MISO → ESP32 GPIO19 (shared with LoRa)
- SCK → ESP32 GPIO18 (shared with LoRa)
- CS → ESP32 GPIO2

```
ESP32          SD Card Module
┌─────┐        ┌──────────────┐
│ 3V3 │────────│ VCC          │
│ GND │────────│ GND          │
│ G23 │────────│ MOSI         │
│ G19 │────────│ MISO         │
│ G18 │────────│ SCK          │
│ G2  │────────│ CS           │
└─────┘        └──────────────┘
```

**Notes:**
- Format SD card as FAT32 before use
- Use quality SD card (Class 10 recommended)
- Some modules have level shifters, others require 3.3V

### 5. 4x4 Matrix Keypad

**Keypad Layout:**
```
    Col1  Col2  Col3  Col4
     │     │     │     │
Row1─┼─ 1 ─┼─ 2 ─┼─ 3 ─┼─ A
Row2─┼─ 4 ─┼─ 5 ─┼─ 6 ─┼─ B  
Row3─┼─ 7 ─┼─ 8 ─┼─ 9 ─┼─ C
Row4─┼─ * ─┼─ 0 ─┼─ # ─┼─ D
```

**Connections:**
```
Keypad Pin     ESP32 Pin
┌─────────┐    ┌─────────┐
│ Row 1   │────│ GPIO32  │
│ Row 2   │────│ GPIO33  │
│ Row 3   │────│ GPIO25  │
│ Row 4   │────│ GPIO26  │
│ Col 1   │────│ GPIO27  │
│ Col 2   │────│ GPIO14  │
│ Col 3   │────│ GPIO12  │
│ Col 4   │────│ GPIO13  │
└─────────┘    └─────────┘
```

**Notes:**
- Keypad pins may be labeled differently - verify with multimeter
- Internal pull-up resistors are enabled in software
- Test each key after assembly

## 🔌 Complete Wiring Diagram

```
                    ESP32 Development Board
                    ┌─────────────────────────┐
              3V3 ──│1                      30│── VIN
               EN ──│2                      29│── GND
           GPIO36 ──│3                      28│── GPIO13 ── Keypad Col4
           GPIO39 ──│4                      27│── GPIO12 ── Keypad Col3  
           GPIO34 ──│5                      26│── GPIO14 ── Keypad Col2
           GPIO35 ──│6                      25│── GPIO27 ── Keypad Col1
  Keypad Row1 ── GPIO32 ──│7                      24│── GPIO26 ── Keypad Row4
  Keypad Row2 ── GPIO33 ──│8                      23│── GPIO25 ── Keypad Row3
  Keypad Row3 ── GPIO25 ──│9                      22│── GPIO33
  Keypad Row4 ── GPIO26 ──│10                     21│── GPIO32
           GPIO27 ──│11                     20│── GPIO35
           GPIO14 ──│12                     19│── GPIO34
           GPIO12 ──│13                     18│── GPIO5
              GND ──│14                     17│── GPIO18
           GPIO13 ──│15                     16│── GPIO19
                    └─────────────────────────┘
                              │
                    ┌─────────┼─────────┐
                    │         │         │
              ┌─────▼───┐ ┌───▼───┐ ┌───▼────┐
              │ OLED    │ │ GPS   │ │ LoRa   │
              │ SH1106  │ │NEO-M8N│ │SX1278  │
              └─────────┘ └───────┘ └────────┘
```

## 🛠️ Assembly Steps

### Step 1: Prepare Components
1. Test all modules individually before assembly
2. Check voltage requirements for each module
3. Prepare jumper wires of appropriate lengths

### Step 2: Power Connections First
1. Connect all VCC pins to ESP32 3.3V rail
2. Connect all GND pins to ESP32 GND rail
3. **Double-check voltage levels before powering on**

### Step 3: Communication Buses
1. Wire I2C bus (OLED display)
2. Wire UART bus (GPS module)  
3. Wire SPI bus (LoRa and SD card)
4. Wire keypad matrix

### Step 4: Testing
1. Connect ESP32 to computer via USB
2. Upload test sketch to verify each module
3. Check serial monitor for initialization messages
4. Test each function individually

## 🔍 Testing Procedures

### Power Test
```cpp
void setup() {
  Serial.begin(115200);
  Serial.println("Power test - all modules should initialize");
}
```

### I2C Scanner (OLED Test)
```cpp
#include <Wire.h>
void setup() {
  Wire.begin(21, 22); // SDA, SCL
  Serial.begin(115200);
  Serial.println("I2C Scanner");
}
void loop() {
  for(byte i = 1; i < 127; i++) {
    Wire.beginTransmission(i);
    if(Wire.endTransmission() == 0) {
      Serial.print("Found I2C device at 0x");
      Serial.println(i, HEX);
    }
  }
  delay(5000);
}
```

### GPS Test
```cpp
#include <HardwareSerial.h>
HardwareSerial gpsSerial(2);
void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);
  Serial.println("GPS Test - should see NMEA sentences");
}
void loop() {
  while(gpsSerial.available()) {
    Serial.write(gpsSerial.read());
  }
}
```

## ⚠️ Common Mistakes

### Power Issues
- **Using 5V for LoRa module** - Will destroy the module!
- **Insufficient power supply** - Use quality USB cable/power source
- **Mixed voltage levels** - Verify each module's requirements

### Wiring Errors
- **Swapped TX/RX on GPS** - GPS TX goes to ESP32 RX pin
- **Wrong SPI connections** - MOSI/MISO easily confused
- **Loose connections** - Ensure solid connections for reliability

### Software Issues
- **Wrong pin definitions** - Verify pin numbers match physical connections
- **Missing pull-ups** - Some modules need external pull-up resistors
- **Incorrect baud rates** - GPS typically uses 9600 baud

## 🔧 Troubleshooting

### No Display Output
1. Check I2C address (try 0x3C and 0x3D)
2. Verify SDA/SCL connections
3. Test with I2C scanner code
4. Check power supply voltage

### GPS Not Working
1. Move to open area for testing
2. Check TX/RX connections (commonly swapped)
3. Verify baud rate (9600 for most modules)
4. Wait 2-5 minutes for initial fix

### LoRa Communication Failed
1. **Verify 3.3V power supply**
2. Check antenna connection
3. Verify SPI connections
4. Test with simple LoRa sender/receiver code
5. Check frequency setting matches other devices

### SD Card Issues
1. Format as FAT32
2. Check SPI connections (shared with LoRa)
3. Verify CS pin connection
4. Try different SD card

### Keypad Not Responding
1. Test continuity with multimeter
2. Verify row/column pin assignments
3. Check for short circuits
4. Test individual keys

## 📦 Enclosure Considerations

### Mechanical Design
- Provide access to keypad and display
- Allow antenna connections to extend outside
- Include ventilation for heat dissipation
- Consider waterproofing for outdoor use

### Antenna Placement
- Keep LoRa antenna away from other electronics
- Position GPS antenna with clear sky view
- Use proper antenna connectors (SMA/U.FL)
- Consider external antenna mounting

### Power Management
- Include power switch and LED indicator
- Provide access to USB charging port
- Consider battery level monitoring
- Plan for external power connections

---

**⚠️ Safety Reminders:**
- Always verify voltage levels before connecting power
- Use proper ESD precautions when handling modules
- Double-check connections before powering on
- Keep antennas away from body during transmission
