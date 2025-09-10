# ESP32 Mesh Communicator

A comprehensive offline communication device built on ESP32 that enables mesh networking, GPS tracking, and emergency broadcasting capabilities. Perfect for remote areas, disaster scenarios, or situations where traditional communication infrastructure is unavailable.

## 🚀 Features

### Core Communication
- **LoRa Mesh Networking**: Long-range communication up to 10km+ in open areas
- **Message Acknowledgment System**: Reliable delivery with automatic retry mechanism
- **Dual Frequency Support**: 433MHz and 169MHz bands
- **T9-Style Text Input**: Efficient message composition using 4x4 keypad

### Location Services
- **GPS Integration**: Real-time location tracking with NEO-M8N module
- **Location Sharing**: Attach GPS coordinates to messages
- **Last Known Location**: Automatic saving of GPS data when signal is lost

### Emergency Features
- **SOS Broadcasting**: Automatic emergency message transmission every 10 seconds
- **Emergency Location**: GPS coordinates included in SOS messages
- **Visual/Audio Alerts**: LED and buzzer notifications

### Connectivity & Storage
- **WiFi Web Interface**: Remote control and monitoring via web browser
- **SD Card Logging**: Persistent message storage and retrieval
- **Message Management**: View, delete, forward, and reply to messages
- **EEPROM Settings**: Persistent configuration storage

### User Interface
- **OLED Display**: 128x64 SH1106 display with intuitive menus
- **4x4 Keypad**: Physical input with T9 text entry
- **Multi-State Navigation**: Comprehensive menu system
- **Real-time Status**: GPS, LoRa, WiFi, and battery indicators

## 📋 Hardware Requirements

### Core Components
- **ESP32 Development Board** (ESP32-WROOM-32)
- **SX1278 LoRa Module** (433MHz or 169MHz)
- **NEO-M8N GPS Module** with antenna
- **SH1106 OLED Display** (128x64, I2C)
- **4x4 Matrix Keypad**
- **MicroSD Card Module**

### Optional Components
- **Active Buzzer** (5V)
- **Status LEDs** (3x)
- **External Antenna** for LoRa (improves range significantly)
- **Battery Pack** (18650 Li-ion recommended)

## 🔌 Pin Configuration

### ESP32 Pin Assignments
```
OLED Display (SH1106):
- SDA: GPIO 21
- SCL: GPIO 22
- VCC: 3.3V
- GND: GND

GPS Module (NEO-M8N):
- RX: GPIO 16
- TX: GPIO 17
- VCC: 3.3V
- GND: GND

LoRa Module (SX1278):
- CS:   GPIO 5
- RST:  GPIO 15
- DIO0: GPIO 4
- MOSI: GPIO 23
- MISO: GPIO 19
- SCK:  GPIO 18
- VCC:  3.3V
- GND:  GND

SD Card Module:
- CS:   GPIO 2
- MOSI: GPIO 23
- MISO: GPIO 19
- SCK:  GPIO 18
- VCC:  3.3V
- GND:  GND

4x4 Keypad:
- Row 1: GPIO 32
- Row 2: GPIO 33
- Row 3: GPIO 25
- Row 4: GPIO 26
- Col 1: GPIO 27
- Col 2: GPIO 14
- Col 3: GPIO 12
- Col 4: GPIO 13
```

## 📚 Required Libraries

Install these libraries through Arduino IDE Library Manager:

```cpp
// Core Libraries
#include <WiFi.h>          // ESP32 built-in
#include <WebServer.h>     // ESP32 built-in
#include <EEPROM.h>        // ESP32 built-in
#include <SPI.h>           // Arduino built-in
#include <Wire.h>          // Arduino built-in

// External Libraries (install via Library Manager)
#include <U8g2lib.h>       // U8g2 by oliver
#include <TinyGPS++.h>     // TinyGPS++ by Mikal Hart
#include <Keypad.h>        // Keypad by Mark Stanley
#include <LoRa.h>          // LoRa by Sandeep Mistry
#include <SD.h>            // SD by Arduino
```

## 🛠️ Installation & Setup

### 1. Hardware Assembly
1. Connect all components according to the pin configuration above
2. Ensure proper power supply (3.3V for most modules, 5V for ESP32)
3. Install antennas for LoRa and GPS modules
4. Insert formatted microSD card

### 2. Software Setup
1. Install Arduino IDE (version 1.8.19 or later)
2. Add ESP32 board support:
   - File → Preferences → Additional Board Manager URLs
   - Add: `https://dl.espressif.com/dl/package_esp32_index.json`
   - Tools → Board → Boards Manager → Search "ESP32" → Install
3. Install required libraries (see list above)
4. Select board: "ESP32 Dev Module"
5. Upload the sketch to your ESP32

### 3. Initial Configuration
1. Power on the device
2. Navigate to Settings → LoRa Frequency → Select your region's frequency
3. (Optional) Configure WiFi for web interface access
4. Test GPS reception in open area
5. Verify LoRa communication with another device

## 🎮 Usage Guide

### Basic Operation
- **Navigation**: Use keypad numbers 2/8 for up/down, 5 to select
- **Text Input**: Press * to toggle between numbers and letters mode
- **Back/Cancel**: Press B or D to go back
- **Send Message**: Compose message, press A to send

### Menu Structure
```
Main Menu
├── Send Message      (Compose and send LoRa messages)
├── View Messages     (Browse received/sent messages)
├── GPS              (View current location and save waypoints)
├── Settings         (Configure device settings)
└── SOS Emergency    (Emergency broadcasting mode)

Settings Menu
├── WiFi Setup       (Configure wireless connection)
├── LoRa Frequency   (Select 433MHz or 169MHz)
├── Reset Messages   (Clear all stored messages)
├── WiFi Disconnect  (Disable wireless)
└── Device Info      (System information)
```

### Message Features
- **GPS Attachment**: Press D while composing to include location
- **Message Types**: View All, Received only, or Sent only
- **Message Actions**: Reply, Delete, Forward
- **Acknowledgments**: Automatic delivery confirmation

### Web Interface
1. Connect device to WiFi network
2. Note the IP address displayed on OLED
3. Open web browser and navigate to the IP address
4. Access full device control and message history

## 🆘 Emergency Features

### SOS Mode
- Activates automatic emergency broadcasting
- Sends distress message every 10 seconds
- Includes GPS coordinates when available
- Visual indicator shows active status
- Can be activated from main menu or web interface

### Emergency Message Format
```
SOS: EMERGENCY - Need assistance at [GPS coordinates]
Timestamp: [device uptime]
Device ID: [unique identifier]
```

## 🔧 Troubleshooting

### Common Issues

**LoRa Not Working**
- Check antenna connection
- Verify frequency setting matches other devices
- Ensure proper power supply (3.3V)
- Check SPI connections

**GPS No Fix**
- Move to open area away from buildings
- Wait 2-5 minutes for initial fix
- Check antenna connection
- Verify 3.3V power supply

**Display Issues**
- Check I2C connections (SDA/SCL)
- Verify display address (usually 0x3C)
- Ensure proper power supply

**SD Card Problems**
- Format card as FAT32
- Check SPI connections
- Verify card is properly inserted
- Try different SD card

### Performance Optimization
- Use external antennas for better range
- Position GPS antenna with clear sky view
- Keep LoRa modules away from interference sources
- Use quality power supply for stable operation

## 📡 Communication Protocol

### Message Format
```
Regular Message: MSG:[ID]:[content]
With GPS: MSG:[ID]:[content]|[lat,lon,satellites]
Acknowledgment: ACK:[ID]
SOS Message: SOS:[ID]:[emergency_content]|[GPS_data]
```

### Network Topology
- Peer-to-peer mesh networking
- Automatic message relay (future enhancement)
- Collision avoidance with random delays
- Acknowledgment-based reliability

## 🔋 Power Management

### Power Consumption
- Active mode: ~200-300mA
- Sleep mode: ~10-50mA (future enhancement)
- GPS acquisition: +50mA
- LoRa transmission: +120mA (peak)

### Battery Life Estimates
- 2000mAh battery: 6-10 hours active use
- 5000mAh battery: 15-25 hours active use
- Power bank: Extended operation possible

## 🤝 Contributing

1. Fork the repository
2. Create feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open Pull Request

### Development Guidelines
- Follow Arduino coding standards
- Comment complex functions
- Test on actual hardware
- Update documentation for new features

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **LoRa Library**: Sandeep Mistry
- **U8g2 Display Library**: Oliver Kraus
- **TinyGPS++**: Mikal Hart
- **Keypad Library**: Mark Stanley
- **ESP32 Community**: For extensive documentation and support

## 📞 Support

- **Issues**: Report bugs via GitHub Issues
- **Discussions**: Join community discussions
- **Documentation**: Check wiki for additional guides
- **Hardware**: Verify connections with provided wiring diagrams

## 🔮 Future Enhancements

- [ ] Mesh routing and message relay
- [ ] Encryption for secure communications
- [ ] Power management and sleep modes
- [ ] Mobile app companion
- [ ] Weather station integration
- [ ] Voice message recording
- [ ] Bluetooth connectivity
- [ ] Solar charging support

---

**⚠️ Important Notes:**
- Verify local regulations for LoRa frequency usage
- This device is for emergency and educational use
- Range varies significantly based on terrain and obstacles
- GPS accuracy depends on satellite visibility and atmospheric conditions
