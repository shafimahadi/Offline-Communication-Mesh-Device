# Libraries and Dependencies

This document provides detailed information about all required libraries and dependencies for the ESP32 Mesh Communicator project.

## 📚 Required Libraries

### Core ESP32 Libraries (Built-in)
These libraries come pre-installed with the ESP32 Arduino Core:

```cpp
#include <WiFi.h>          // WiFi connectivity
#include <WebServer.h>     // HTTP web server
#include <EEPROM.h>        // Non-volatile storage
#include <SPI.h>           // SPI communication protocol
#include <Wire.h>          // I2C communication protocol
#include <SD.h>            // SD card file system
```

### External Libraries (Install via Library Manager)

#### 1. U8g2 Display Library
- **Library Name**: U8g2
- **Author**: oliver
- **Version**: 2.34.x or later
- **Purpose**: OLED display control (SH1106/SSD1306)
- **Installation**: Arduino IDE → Tools → Manage Libraries → Search "U8g2"

```cpp
#include <U8g2lib.h>
```

**Key Features:**
- Support for monochrome displays
- Multiple font options
- Graphics primitives
- Memory-efficient buffering

#### 2. TinyGPS++ Library
- **Library Name**: TinyGPS++
- **Author**: Mikal Hart
- **Version**: 1.0.x or later
- **Purpose**: GPS NMEA sentence parsing
- **Installation**: Arduino IDE → Tools → Manage Libraries → Search "TinyGPS++"

```cpp
#include <TinyGPS++.h>
```

**Key Features:**
- NMEA sentence parsing
- Location, time, speed extraction
- Satellite information
- Course and altitude data

#### 3. Keypad Library
- **Library Name**: Keypad
- **Author**: Mark Stanley, Alexander Brevig
- **Version**: 3.1.x or later
- **Purpose**: Matrix keypad input handling
- **Installation**: Arduino IDE → Tools → Manage Libraries → Search "Keypad"

```cpp
#include <Keypad.h>
```

**Key Features:**
- Matrix keypad scanning
- Debouncing
- Multiple key press detection
- Customizable key mapping

#### 4. LoRa Library
- **Library Name**: LoRa
- **Author**: Sandeep Mistry
- **Version**: 0.8.x or later
- **Purpose**: LoRa radio communication
- **Installation**: Arduino IDE → Tools → Manage Libraries → Search "LoRa"

```cpp
#include <LoRa.h>
```

**Key Features:**
- SX127x chip support
- Configurable parameters
- Interrupt-driven reception
- CRC error checking

## 🛠️ Installation Instructions

### Method 1: Arduino IDE Library Manager (Recommended)

1. Open Arduino IDE
2. Go to **Tools** → **Manage Libraries**
3. Search for each library by name
4. Click **Install** for the latest version
5. Wait for installation to complete

### Method 2: Manual Installation

1. Download library ZIP files from GitHub
2. Go to **Sketch** → **Include Library** → **Add .ZIP Library**
3. Select the downloaded ZIP file
4. Restart Arduino IDE

### Method 3: Git Clone (Advanced)

```bash
cd ~/Arduino/libraries/
git clone https://github.com/olikraus/u8g2.git
git clone https://github.com/mikalhart/TinyGPSPlus.git
git clone https://github.com/Chris--A/Keypad.git
git clone https://github.com/sandeepmistry/arduino-LoRa.git
```

## 📋 Library Versions and Compatibility

| Library | Minimum Version | Tested Version | ESP32 Compatible |
|---------|----------------|----------------|-------------------|
| U8g2 | 2.28.0 | 2.34.22 | ✅ |
| TinyGPS++ | 1.0.0 | 1.0.3 | ✅ |
| Keypad | 3.1.0 | 3.1.1 | ✅ |
| LoRa | 0.8.0 | 0.8.0 | ✅ |
| WiFi | Built-in | 2.0.0 | ✅ |
| WebServer | Built-in | 2.0.0 | ✅ |
| EEPROM | Built-in | 2.0.0 | ✅ |
| SPI | Built-in | 2.0.0 | ✅ |
| Wire | Built-in | 2.0.0 | ✅ |
| SD | Built-in | 2.0.0 | ✅ |

## 🔧 Configuration and Setup

### U8g2 Display Configuration

The project uses SH1106 OLED display with I2C interface:

```cpp
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);
```

**Alternative displays supported:**
- SSD1306: `U8G2_SSD1306_128X64_NONAME_F_HW_I2C`
- SH1107: `U8G2_SH1107_128X128_F_HW_I2C`

### TinyGPS++ Configuration

```cpp
TinyGPSPlus gps;
HardwareSerial gpsSerial(2); // Use UART2
```

**Baud rate settings:**
- Most GPS modules: 9600 baud
- Some modules: 38400 baud
- Configure in setup(): `gpsSerial.begin(9600, SERIAL_8N1, 16, 17);`

### Keypad Configuration

```cpp
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {32, 33, 25, 26};
byte colPins[COLS] = {27, 14, 12, 13};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
```

### LoRa Configuration

```cpp
// Pin definitions
#define LORA_CS_PIN 5
#define LORA_RST_PIN 15
#define LORA_DIO0_PIN 4

// Initialize LoRa
LoRa.setPins(LORA_CS_PIN, LORA_RST_PIN, LORA_DIO0_PIN);
LoRa.begin(433E6); // 433MHz frequency
```

## 🚨 Common Installation Issues

### Library Not Found
**Problem**: `fatal error: LibraryName.h: No such file or directory`

**Solutions:**
1. Verify library is installed via Library Manager
2. Check library name spelling in `#include` statement
3. Restart Arduino IDE after installation
4. Clear Arduino IDE cache: Close IDE, delete `%APPDATA%\Arduino15\packages` (Windows) or `~/Library/Arduino15/packages` (Mac)

### Version Conflicts
**Problem**: Compilation errors due to library version mismatches

**Solutions:**
1. Update all libraries to latest versions
2. Check library compatibility matrix above
3. Use specific library versions if needed
4. Remove conflicting library versions manually

### ESP32 Board Package Issues
**Problem**: ESP32-specific functions not available

**Solutions:**
1. Install ESP32 board package: `https://dl.espressif.com/dl/package_esp32_index.json`
2. Select correct board: "ESP32 Dev Module"
3. Update ESP32 core to latest version
4. Verify board selection in Tools menu

### Memory Issues
**Problem**: Sketch too large or insufficient RAM

**Solutions:**
1. Enable PSRAM if available: Tools → PSRAM → "Enabled"
2. Optimize display buffer usage in U8g2
3. Reduce string constants and arrays
4. Use PROGMEM for constant data

## 📊 Memory Usage Analysis

### Flash Memory Usage (Approximate)
- Core ESP32 libraries: ~200KB
- U8g2 library: ~150KB
- LoRa library: ~50KB
- TinyGPS++ library: ~20KB
- Keypad library: ~10KB
- Project code: ~100KB
- **Total**: ~530KB (fits in 4MB ESP32 flash)

### RAM Usage (Approximate)
- Display buffer: ~1KB
- Message buffers: ~2KB
- GPS data structures: ~1KB
- WiFi stack: ~40KB
- LoRa buffers: ~1KB
- Variables and stack: ~10KB
- **Total**: ~55KB (fits in 320KB ESP32 RAM)

## 🔄 Library Updates

### Checking for Updates
```bash
# Using Arduino CLI
arduino-cli lib list
arduino-cli lib upgrade
```

### Manual Update Process
1. Open Arduino IDE
2. Go to Tools → Manage Libraries
3. Filter by "Updatable"
4. Update libraries individually or all at once
5. Test project after updates

### Version Pinning (Advanced)
For production deployments, consider pinning library versions:

```json
// library.properties example
name=ESP32 Mesh Communicator
version=1.0.0
dependencies=U8g2@2.34.22,TinyGPS++@1.0.3,Keypad@3.1.1,LoRa@0.8.0
```

## 🧪 Testing Library Installation

### Quick Test Sketch
```cpp
#include <U8g2lib.h>
#include <TinyGPS++.h>
#include <Keypad.h>
#include <LoRa.h>
#include <WiFi.h>
#include <SD.h>

void setup() {
  Serial.begin(115200);
  Serial.println("Library Test");
  
  // Test each library
  Serial.println("U8g2: " + String(U8G2_VERSION));
  Serial.println("TinyGPS++: Available");
  Serial.println("Keypad: Available");
  Serial.println("LoRa: Available");
  Serial.println("WiFi: " + WiFi.macAddress());
  Serial.println("All libraries loaded successfully!");
}

void loop() {
  // Empty loop
}
```

### Expected Output
```
Library Test
U8g2: 10834
TinyGPS++: Available
Keypad: Available
LoRa: Available
WiFi: XX:XX:XX:XX:XX:XX
All libraries loaded successfully!
```

## 📖 Additional Resources

### Documentation Links
- [U8g2 Reference](https://github.com/olikraus/u8g2/wiki)
- [TinyGPS++ Documentation](http://arduiniana.org/libraries/tinygpsplus/)
- [Keypad Library Guide](https://playground.arduino.cc/Code/Keypad/)
- [LoRa Library Examples](https://github.com/sandeepmistry/arduino-LoRa/tree/master/examples)
- [ESP32 Arduino Core](https://docs.espressif.com/projects/arduino-esp32/en/latest/)

### Community Support
- [Arduino Forum](https://forum.arduino.cc/)
- [ESP32 Community](https://www.esp32.com/)
- [GitHub Issues](https://github.com/) for specific library problems

### Alternative Libraries
If you encounter issues with the recommended libraries, consider these alternatives:

| Function | Primary Library | Alternative |
|----------|----------------|-------------|
| Display | U8g2 | Adafruit_SSD1306 |
| GPS | TinyGPS++ | SoftwareSerial + manual parsing |
| Keypad | Keypad | Custom matrix scanning |
| LoRa | LoRa (Sandeep Mistry) | RadioHead |

---

**💡 Pro Tips:**
- Always test library installation with simple examples first
- Keep library versions documented for reproducible builds
- Monitor library update changelogs for breaking changes
- Use version control to track library dependencies
