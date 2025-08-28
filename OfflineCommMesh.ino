#include <Wire.h>
#include <U8g2lib.h>
#include <TinyGPS++.h>
#include <Keypad.h>
#include <SPI.h>
#include <LoRa.h>
#include <SD.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>

// --- Hardware Pin Definitions ---
// OLED (SH1106)
#define OLED_SDA 21
#define OLED_SCL 22

// GPS (NEO-M8N)
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17

// LoRa (SX1278)
#define LORA_CS_PIN 5
#define LORA_RST_PIN 15
#define LORA_DIO0_PIN 4

// SD Card Module
#define SD_CS_PIN 2

// Keypad (4x4)
byte ROW_PINS[4] = {32, 33, 25, 26};
byte COL_PINS[4] = {27, 14, 12, 13};
const char KEY_MAP[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

// --- Object Instantiation ---
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);
TinyGPSPlus gps;
Keypad keypad = Keypad(makeKeymap(KEY_MAP), ROW_PINS, COL_PINS, 4, 4);

// --- State Management ---
enum DeviceState {
    STATE_MAIN_MENU,
    STATE_SEND_MESSAGE,
    STATE_GPS_INFO,
    STATE_SETTINGS,
    STATE_LORA_RX,
    STATE_WAITING_ACK,
    STATE_VIEW_MESSAGES,
    STATE_MESSAGE_DETAIL,
    STATE_MESSAGE_OPTIONS,
    STATE_DELETE_CONFIRM,
    STATE_FORWARD_MESSAGE,
    STATE_VIEW_GPS_LOCATION,
    STATE_WIFI_SCAN,
    STATE_WIFI_SELECT,
    STATE_WIFI_PASSWORD,
    STATE_WEB_INTERFACE,
    STATE_RESET_CONFIRM,
    STATE_SOS_EMERGENCY,
    STATE_FREQUENCY_SELECT
};
DeviceState currentState = STATE_MAIN_MENU;

// --- Menu Variables ---
int menuIndex = 0;
const char *menuItems[] = {"Send Message", "View Messages", "GPS", "Settings", "SOS Emergency"};
const int MENU_COUNT = 5;

// --- Settings Menu Variables ---
int settingsIndex = 0;
const char *settingsItems[] = {"WiFi Setup", "LoRa Frequency", "Reset Messages", "WiFi Disconnect", "Device Info"};
const int SETTINGS_COUNT = 5;

// --- Frequency Menu Variables ---
int frequencyIndex = 0;
const char *frequencyItems[] = {"433 MHz", "169 MHz"};
const int FREQUENCY_COUNT = 2;

// WiFi Settings
char wifiSSID[32] = "";
char wifiPassword[64] = "";
bool wifiConfigured = false;
IPAddress localIP;
WebServer server(80);

// --- Message Buffer for Text Input ---
char fullMessage[128] = "";
int msgIndex = 0;

// --- Input Mode Variables ---
int inputMode = 0; // 0 = Numbers, 1 = Letters
const char *inputModeNames[] = {"Numbers", "Letters"};

// --- Multi-Letter Input Logic Variables ---
const char *keyMapLetters[10] = {"", "", "ABC", "DEF", "GHI", "JKL", "MNO", "PQRS", "TUV", "WXYZ"};
int lastKey = -1;
int currentLetterIndex = 0;
unsigned long lastPress = 0;
const unsigned long LETTER_TIMEOUT = 3000;

// --- LoRa Receive Variables ---
volatile bool newMessageReceived = false;
char receivedMessage[256] = "";
char receivedId[32] = "";
char receivedGPSData[64] = "";

// --- Message Acknowledgment System ---
unsigned long lastSendTime = 0;
const unsigned long ACK_TIMEOUT = 5000;
const unsigned long RETRY_INTERVAL = 2000;
int retryCount = 0;
const int MAX_RETRIES = 3;
char pendingMessageId[32] = "";
char pendingMessage[128] = "";

// --- Watchdog and Stability Variables ---
unsigned long lastActivityTime = 0;
const unsigned long WATCHDOG_TIMEOUT = 30000;
bool loraInitialized = false;

// --- Message Viewing Variables ---
int messageIndex = 0;
int totalMessages = 0;
int viewOffset = 0;
int receivedCount = 0;
int sentCount = 0;
int currentMessageType = 0; // 0=All, 1=Received, 2=Sent
const int MESSAGES_PER_PAGE = 4;
const int MAX_MESSAGE_PREVIEW = 16;
const int MAX_MESSAGES = 50;

// --- GPS Sharing Variables ---
bool includeGPS = false;
bool gpsFixAvailable = false;
char currentGPSData[64] = "";
char lastKnownGPSData[64] = "";
bool hasLastKnownGPS = false;

// Structure to store message info
struct MessageInfo {
    long position;
    char type; // 'R'=Received, 'S'=Sent, 'A'=Ack, 'F'=Failed
    char preview[21];
    bool hasGPS;
};

MessageInfo messageList[50]; // Store up to 50 messages
int messageCount = 0;

// --- Forward Message Variables ---
char forwardMessage[128] = "";

// --- WiFi Setup Variables ---
char wifiInputBuffer[64] = "";
int wifiInputIndex = 0;
int wifiSetupStep = 0; // 0 = SSID, 1 = Password

// --- WiFi Scanning Variables ---
String wifiNetworks[10]; // Store up to 10 networks
int wifiNetworkCount = 0;
int selectedNetworkIndex = 0;
bool wifiScanComplete = false;

// --- WiFi Password Input Variables ---
int wifiInputMode = 0; // 0 = Numbers, 1 = Letters
int wifiLastKey = -1;
int wifiCurrentLetterIndex = 0;
unsigned long wifiLastPress = 0;
bool wifiUpperCase = false; // false = lowercase, true = uppercase

// --- SOS Emergency Variables ---
bool sosActive = false;
unsigned long lastSOSTransmission = 0;
const unsigned long SOS_INTERVAL = 10000; // Send SOS every 10 seconds
int sosCounter = 0;

// --- LoRa Frequency Variables ---
int loraFrequency = 0; // 0 = 433MHz, 1 = 169MHz
const long FREQ_433 = 433E6;
const long FREQ_169 = 169E6;
const char* freqNames[] = {"433 MHz", "169 MHz"};
const int EEPROM_FREQ_ADDR = 100; // EEPROM address for frequency setting

// --- Function Prototypes ---
void showMenu();
void executeState();
void handleKeypadInput(char key);
void handleMessageInput(char key, unsigned long now);
bool initLoRa();
bool initSDCard();
void onReceive(int packetSize);
void generateMessageId(char* idBuffer, size_t bufferSize);
void sendMessageWithRetry();
void checkForAck();
void resetWatchdog();
void checkWatchdog();
void safeStringCopy(char* dest, const char* src, size_t destSize);
void safeStringConcat(char* dest, const char* src, size_t destSize);
void loadMessageList();
void viewMessagesScreen();
void viewMessageDetail();
void viewMessageOptions();
void viewDeleteConfirm();
void viewForwardMessage();
void viewGPSLocation();
String getMessageTypeName(int type);
bool deleteMessageFromSD(int index);
bool extractMessageContent(const char* logLine, char* contentBuffer, size_t bufferSize);
void updateGPSData();
void saveGPSToSD();
void loadGPSFromSD();
void showWifiSetupScreen();
void handleWifiInput(char key);
void saveWifiCredentials();
void loadWifiCredentials();
void setupWebServer();
void handleRoot();
void handleSendMessage();
void handleGetMessages();
void handleGetStatus();
String getMessagesJSON();
void showSettingsMenu(); // Added function prototype
void handleSetFrequency();
void handleGetFrequency();
void scanWifiNetworks();
void showWifiScanScreen();
void showWifiSelectScreen();
void showWifiPasswordScreen();
void handleWifiScanInput(char key);
void handleWifiSelectInput(char key);
void handleWifiPasswordInput(char key);
void handleWifiTextInput(char key, unsigned long now);
void showResetConfirmScreen();
void resetAllMessages();
void showSOSScreen();
void startSOS();
void stopSOS();
void sendSOSMessage();
void showFrequencySelectScreen();
void handleFrequencySelectInput(char key);
void saveFrequencySettings();
void loadFrequencySettings();
void setLoRaFrequency();

// --- Utility Functions ---
void safeStringCopy(char* dest, const char* src, size_t destSize) {
    if (destSize > 0) {
        strncpy(dest, src, destSize - 1);
        dest[destSize - 1] = '\0';
    }
}

void safeStringConcat(char* dest, const char* src, size_t destSize) {
    if (destSize > 0) {
        strncat(dest, src, destSize - strlen(dest) - 1);
    }
}

void generateMessageId(char* idBuffer, size_t bufferSize) {
    static long counter = 0;
    snprintf(idBuffer, bufferSize, "%lu-%d-%ld", millis(), random(1000), counter++);
}

void resetWatchdog() {
    lastActivityTime = millis();
}

void checkWatchdog() {
    if (millis() - lastActivityTime > WATCHDOG_TIMEOUT) {
        // Watchdog timeout, resetting LoRa
        
        // Reset LoRa module
        digitalWrite(LORA_RST_PIN, LOW);
        delay(10);
        digitalWrite(LORA_RST_PIN, HIGH);
        delay(10);
        
        // Reinitialize LoRa
        initLoRa();
        
        resetWatchdog();
    }
}

// --- LoRa Functions ---
bool initLoRa() {
    SPI.begin(18, 19, 23, LORA_CS_PIN);
    LoRa.setPins(LORA_CS_PIN, LORA_RST_PIN, LORA_DIO0_PIN);
    
    // Reset LoRa module first
    digitalWrite(LORA_RST_PIN, LOW);
    delay(10);
    digitalWrite(LORA_RST_PIN, HIGH);
    delay(10);
    
    // Use selected frequency
    long frequency = (loraFrequency == 0) ? FREQ_433 : FREQ_169;
    if (!LoRa.begin(frequency)) {
        // LoRa init failed
        loraInitialized = false;
        return false;
    }
    
    // LoRa configuration
    LoRa.setSpreadingFactor(7);       // Range: 6-12
    LoRa.setSignalBandwidth(125E3);   // 125 kHz
    LoRa.setCodingRate4(5);           // 4/5 coding rate
    LoRa.setPreambleLength(8);        // 8 symbols preamble
    
    LoRa.enableCrc();
    LoRa.setSyncWord(0xF3); // Must match on both transmitter and receiver
    
    LoRa.onReceive(onReceive);
    LoRa.receive();
    // LoRa init successful
    loraInitialized = true;
    return true;
}

void onReceive(int packetSize) {
    if (packetSize == 0) return;
    
    // Clear buffers
    memset(receivedMessage, 0, sizeof(receivedMessage));
    memset(receivedGPSData, 0, sizeof(receivedGPSData));
    
    // Read packet
    int i = 0;
    while (LoRa.available() && i < sizeof(receivedMessage) - 1) {
        receivedMessage[i++] = (char)LoRa.read();
    }
    receivedMessage[i] = '\0';
    
    newMessageReceived = true;
}

void sendMessageWithRetry() {
    if (millis() - lastSendTime < RETRY_INTERVAL && lastSendTime != 0) {
        return; // Not time to retry yet
    }
    
    if (retryCount >= MAX_RETRIES) {
        // Max retries reached, give up
        display.clearBuffer();
        display.drawStr(0, 15, "Send failed!");
        display.drawStr(0, 30, "No acknowledgment");
        display.drawStr(0, 60, "B=Back");
        display.sendBuffer();
        
        // Log failure to SD card
        File logFile = SD.open("/log.txt", FILE_APPEND);
        if (logFile) {
            logFile.println("FAILED: " + String(pendingMessage));
            logFile.close();
        }
        
        delay(2000);
        currentState = STATE_MAIN_MENU;
        showMenu();
        return;
    }
    
    // Build the message with optional GPS data
    char messageToSend[256];
    if (includeGPS) {
        if (gpsFixAvailable) {
            // Format: MSG:ID:message|LAT,LON,SAT (live GPS)
            snprintf(messageToSend, sizeof(messageToSend), "MSG:%s:%s|%.6f,%.6f,%d", 
                     pendingMessageId, pendingMessage, 
                     gps.location.lat(), gps.location.lng(), 
                     gps.satellites.value());
        } else if (strlen(lastKnownGPSData) > 0) {
            // Format: MSG:ID:message|LAST_KNOWN_GPS (saved GPS)
            snprintf(messageToSend, sizeof(messageToSend), "MSG:%s:%s|%s", 
                     pendingMessageId, pendingMessage, lastKnownGPSData);
        } else {
            // No GPS data available
            snprintf(messageToSend, sizeof(messageToSend), "MSG:%s:%s", 
                     pendingMessageId, pendingMessage);
        }
    } else {
        // Regular message without GPS
        snprintf(messageToSend, sizeof(messageToSend), "MSG:%s:%s", 
                 pendingMessageId, pendingMessage);
    }
    
    // Sending message
    
    // Reinitialize LoRa if needed
    if (!loraInitialized) {
        initLoRa();
    }
    
    LoRa.beginPacket();
    LoRa.print(messageToSend);
    LoRa.endPacket();
    
    // Log message to SD card
    File logFile = SD.open("/log.txt", FILE_APPEND);
    if (logFile) {
        if (includeGPS && (gpsFixAvailable || strlen(lastKnownGPSData) > 0)) {
            logFile.println("SENT: " + String(messageToSend) + " [GPS]");
        } else {
            logFile.println("SENT: " + String(messageToSend));
        }
        logFile.close();
    }
    
    lastSendTime = millis();
    retryCount++;
    executeState(); // Update display with new retry count
}

void checkForAck() {
    if (newMessageReceived) {
        newMessageReceived = false;
        
        // Check if this is an acknowledgment for our message
        if (strncmp(receivedMessage, "ACK:", 4) == 0) {
            char ackId[32];
            if (sscanf(receivedMessage, "ACK:%31s", ackId) == 1) {
                if (strcmp(ackId, pendingMessageId) == 0) {
                    // Message acknowledged!
                    display.clearBuffer();
                    display.drawStr(0, 15, "Message delivered!");
                    display.drawStr(0, 30, ("Retries: " + String(retryCount) + "/" + String(MAX_RETRIES)).c_str());
                    display.drawStr(0, 60, "B=Back");
                    display.sendBuffer();
                    
                    // Log success to SD card
                    File logFile = SD.open("/log.txt", FILE_APPEND);
                    if (logFile) {
                        logFile.println("ACK: " + String(pendingMessageId));
                        logFile.close();
                    }
                    
                    delay(2000);
                    currentState = STATE_MAIN_MENU;
                    showMenu();
                    return;
                }
            }
        }
    }
}

// --- SD Card Functions ---
bool initSDCard() {
    // Initializing SD card
    
    if (!SD.begin(SD_CS_PIN)) {
        // SD card failed
        return false;
    }
    
    uint8_t cardType = SD.cardType();
    // SD card type detected
    
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    // SD card init successful
    
    // Create log file if it doesn't exist
    if (!SD.exists("/log.txt")) {
        File logFile = SD.open("/log.txt", FILE_WRITE);
        if (logFile) {
            logFile.println("Message Log Started");
            logFile.close();
        }
    }
    return true;
}

bool extractMessageContent(const char* logLine, char* contentBuffer, size_t bufferSize) {
    if (strstr(logLine, "MSG:") != NULL) {
        // Extract content from MSG:ID:content format
        const char* thirdColon = strchr(logLine, ':');
        if (thirdColon) {
            thirdColon = strchr(thirdColon + 1, ':');
            if (thirdColon) {
                thirdColon = strchr(thirdColon + 1, ':');
                if (thirdColon) {
                    // Check if there's GPS data
                    char* gpsSeparator = strchr(thirdColon + 1, '|');
                    if (gpsSeparator) {
                        // Extract only the message part (before the |)
                        size_t msgLength = gpsSeparator - (thirdColon + 1);
                        strncpy(contentBuffer, thirdColon + 1, msgLength);
                        contentBuffer[msgLength] = '\0';
                    } else {
                        strncpy(contentBuffer, thirdColon + 1, bufferSize - 1);
                        contentBuffer[bufferSize - 1] = '\0';
                    }
                    return true;
                }
            }
        }
    } else if (strstr(logLine, "RCVD:") != NULL || strstr(logLine, "Received:") != NULL) {
        // Extract content from RCVD:content or Received:content format
        const char* colon = strchr(logLine, ':');
        if (colon) {
            // Check if there's GPS data
            char* gpsSeparator = strchr(colon + 1, '|');
            if (gpsSeparator) {
                // Extract only the message part (before the |)
                size_t msgLength = gpsSeparator - (colon + 1);
                strncpy(contentBuffer, colon + 1, msgLength);
                contentBuffer[msgLength] = '\0';
            } else {
                strncpy(contentBuffer, colon + 1, bufferSize - 1);
                contentBuffer[bufferSize - 1] = '\0';
            }
            return true;
        }
    }
    return false;
}

bool deleteMessageFromSD(int index) {
    if (index < 0 || index >= totalMessages) {
        // Delete failed: Invalid index
        return false;
    }
    
    // Starting delete process
    
    // First, find the exact line to delete by counting only received messages
    File logFile = SD.open("/log.txt", FILE_READ);
    if (!logFile) {
        // Delete failed: Cannot open log file
        return false;
    }
    
    int currentReceivedIndex = 0;
    long deletePosition = -1;
    long position = 0;
    char lineBuffer[256];
    
    while (logFile.available()) {
        long lineStart = position;
        int bytesRead = logFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1);
        lineBuffer[bytesRead] = '\0';
        
        // Check if this is a received message
        if (strstr(lineBuffer, "RCVD:") != NULL || strstr(lineBuffer, "Received:") != NULL) {
            if (currentReceivedIndex == index) {
                deletePosition = lineStart;
                // Found message to delete
                break;
            }
            currentReceivedIndex++;
        }
        position += bytesRead + 1;
    }
    logFile.close();
    
    if (deletePosition == -1) {
        // Delete failed: Could not find message position
        return false;
    }
    
    // Create a temporary file
    File tempFile = SD.open("/temp.txt", FILE_WRITE);
    if (!tempFile) {
        // Delete failed: Cannot create temp file
        return false;
    }
    
    logFile = SD.open("/log.txt", FILE_READ);
    if (!logFile) {
        tempFile.close();
        // Delete failed: Cannot reopen log file
        return false;
    }
    
    // Copy all lines except the one to delete
    position = 0;
    int copiedLines = 0;
    int skippedLines = 0;
    
    while (logFile.available()) {
        long lineStart = position;
        int bytesRead = logFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1);
        lineBuffer[bytesRead] = '\0';
        
        // Skip the line we want to delete
        if (lineStart != deletePosition) {
            tempFile.println(lineBuffer);
            copiedLines++;
        } else {
            skippedLines++;
            // Skipped the target line
        }
        position += bytesRead + 1;
    }
    
    logFile.close();
    tempFile.close();
    
    // Copy operation completed
    
    // Delete original file and rename temp file
    if (SD.remove("/log.txt")) {
        if (SD.rename("/temp.txt", "/log.txt")) {
            // Reload message list
            loadMessageList();
            return true;
        }
    } else {
        // Failed to remove original log file
    }
    
    // Clean up if something went wrong
    SD.remove("/temp.txt");
    // Cleanup completed
    return false;
}

void updateGPSData() {
    if (gps.location.isValid()) {
        snprintf(currentGPSData, sizeof(currentGPSData), "%.6f,%.6f,%d", 
                gps.location.lat(), gps.location.lng(), 
                gps.satellites.value());
        gpsFixAvailable = true;
    } else {
        gpsFixAvailable = false;
        currentGPSData[0] = '\0';
    }
}

void loadMessageList() {
    messageCount = 0;
    receivedCount = 0;
    sentCount = 0;
    
    File logFile = SD.open("/log.txt", FILE_READ);
    if (!logFile) {
        totalMessages = 0;
        // No log file found
        return; // No messages yet
    }
    
    // Loading message list from SD card
    
    long position = 0;
    int allMessagesCount = 0;
    MessageInfo allMessages[100];
    
    while (logFile.available() && allMessagesCount < 100) {
        long lineStart = position;
        char lineBuffer[512]; // Increased buffer size
        int bytesRead = logFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1);
        
        if (bytesRead <= 0) {
            // Skip empty lines
            position += 1;
            continue;
        }
        
        lineBuffer[bytesRead] = '\0';
        position += bytesRead + 1;
        
        // Skip lines that are too short or don't contain message indicators
        if (strlen(lineBuffer) < 5) {
            continue;
        }
        
        // Determine message type with better detection
        char msgType = 'U'; // Unknown
        bool hasGPS = false;
        
        if (strstr(lineBuffer, "RCVD:") != NULL || strstr(lineBuffer, "Received:") != NULL) {
            msgType = 'R';
            receivedCount++;
        } else if (strstr(lineBuffer, "SENT:") != NULL) {
            msgType = 'S';
            sentCount++;
        } else if (strstr(lineBuffer, "ACK:") != NULL) {
            msgType = 'A';
        } else if (strstr(lineBuffer, "FAILED:") != NULL) {
            msgType = 'F';
        } else {
            // Skip lines that don't match any message type
            continue;
        }
        
        // Check if message has GPS data
        if (strstr(lineBuffer, "[GPS]") != NULL || strstr(lineBuffer, "|") != NULL) {
            hasGPS = true;
        }
        
        // Store all messages temporarily
        allMessages[allMessagesCount].position = lineStart;
        allMessages[allMessagesCount].type = msgType;
        allMessages[allMessagesCount].hasGPS = hasGPS;
        
        // Create clean preview - extract only the actual message content
        char cleanContent[MAX_MESSAGE_PREVIEW + 1] = "";
        
        // Skip timestamp at the beginning (everything before first space)
        char* spacePos = strchr(lineBuffer, ' ');
        char* workingLine = spacePos ? spacePos + 1 : lineBuffer;
        
        // Find the actual message content after the type indicator
        char* contentStart = strchr(workingLine, ':');
        if (contentStart) {
            contentStart++; // Skip the colon
            
            // Skip message ID if this is a MSG: format
            if (strstr(workingLine, "MSG:") != NULL) {
                char* secondColon = strchr(contentStart, ':');
                if (secondColon) {
                    contentStart = secondColon + 1; // Skip message ID
                }
            }
            
            // Remove GPS data from preview if present
            char* gpsSeparator = strchr(contentStart, '|');
            if (gpsSeparator) {
                *gpsSeparator = '\0';
            }
            
            // Trim leading/trailing spaces
            while (*contentStart == ' ') contentStart++;
            
            strncpy(cleanContent, contentStart, MAX_MESSAGE_PREVIEW);
            cleanContent[MAX_MESSAGE_PREVIEW] = '\0';
            
            // Add ellipsis if truncated
            if (strlen(contentStart) > MAX_MESSAGE_PREVIEW) {
                cleanContent[MAX_MESSAGE_PREVIEW-2] = '.';
                cleanContent[MAX_MESSAGE_PREVIEW-1] = '.';
            }
        }
        
        // Use clean content or fallback to "Message" if extraction failed
        if (strlen(cleanContent) > 0) {
            strncpy(allMessages[allMessagesCount].preview, cleanContent, MAX_MESSAGE_PREVIEW);
        } else {
            strncpy(allMessages[allMessagesCount].preview, "Message", MAX_MESSAGE_PREVIEW);
        }
        allMessages[allMessagesCount].preview[MAX_MESSAGE_PREVIEW] = '\0';
        
        allMessagesCount++;
    }
    logFile.close();
    
    // Message count completed
    
    // Filter and store in reverse order (newest first)
    messageCount = 0;
    for (int i = allMessagesCount - 1; i >= 0 && messageCount < 50; i--) {
        char msgType = allMessages[i].type;
        
        // Filter based on current view type
        if ((currentMessageType == 0) || 
            (currentMessageType == 1 && msgType == 'R') || 
            (currentMessageType == 2 && msgType == 'S')) {
            
            messageList[messageCount] = allMessages[i];
            messageCount++;
        }
    }
    
    totalMessages = messageCount;
    
    // Message filtering completed
    
    // Ensure indices are valid
    if (messageIndex >= totalMessages) {
        messageIndex = max(0, totalMessages - 1);
    }
    if (viewOffset >= totalMessages) {
        viewOffset = max(0, totalMessages - MESSAGES_PER_PAGE);
    }
}

String getMessageTypeName(int type) {
    switch(type) {
        case 0: return "All";
        case 1: return "Received";
        case 2: return "Sent";
        default: return "All";
    }
}

// --- Display Functions ---
void showMenu() {
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB08_tr);
    
    // Header - compact
    display.drawStr(0, 10, "Offline Comm Mesh");
    display.drawLine(0, 12, 128, 12);
    
    // Menu items with proper spacing
    for (int i = 0; i < MENU_COUNT; i++) {
        int y = 15 + (10 * i); // Reduced spacing
        if (i == menuIndex) {
            display.drawBox(0, y - 2, 128, 10); // Smaller selection box
            display.setDrawColor(0);
            display.drawStr(2, y + 7, menuItems[i]);
            display.setDrawColor(1);
        } else {
            display.drawStr(2, y + 7, menuItems[i]);
        }
    }
    
    display.sendBuffer();
}

void showSettingsMenu() {
    display.clearBuffer();
    display.setFont(u8g2_font_6x10_tr);
    
    display.drawStr(0, 10, "Settings");
    display.drawLine(0, 12, 128, 12);
    
    // Show settings items with navigation
    for (int i = 0; i < SETTINGS_COUNT; i++) {
        int y = 20 + (i * 8);
        if (i == settingsIndex) {
            display.drawStr(0, y, ">");
            display.drawStr(8, y, settingsItems[i]);
        } else {
            display.drawStr(8, y, settingsItems[i]);
        }
    }
    
    display.drawStr(0, 62, "2/8=Nav 5=Select B=Back");
    
    display.sendBuffer();
}

void executeState() {
    switch (currentState) {
        case STATE_MAIN_MENU:
            showMenu();
            break;
        case STATE_SEND_MESSAGE:
            display.clearBuffer();
            display.drawStr(0, 10, "Type message:");
            display.drawStr(0, 22, fullMessage);
            
            // Show input mode and GPS status
            char modeStr[20];
            snprintf(modeStr, sizeof(modeStr), "Mode: %s", inputModeNames[inputMode]);
            display.drawStr(0, 34, modeStr);
            
            if (includeGPS) {
                if (gpsFixAvailable) {
                    display.drawStr(0, 46, "GPS: ON (Live fix)");
                } else if (strlen(lastKnownGPSData) > 0) {
                    display.drawStr(0, 46, "GPS: ON (Last known)");
                } else {
                    display.drawStr(0, 46, "GPS: ON (No data)");
                }
            } else {
                display.drawStr(0, 46, "GPS: OFF");
            }
            
            display.drawStr(0, 58, "A=Send B=Back C=Del D=GPS *=Mode");
            display.sendBuffer();
            break;
        case STATE_GPS_INFO:
            display.clearBuffer();
            if (gps.location.isValid()) {
                char buf[32];
                snprintf(buf, sizeof(buf), "Lat: %.6f", gps.location.lat());
                display.drawStr(0, 12, buf);
                snprintf(buf, sizeof(buf), "Lon: %.6f", gps.location.lng());
                display.drawStr(0, 22, buf);
                snprintf(buf, sizeof(buf), "Sat: %d", gps.satellites.value());
                display.drawStr(0, 32, buf);
                if (gps.time.isValid()) {
                    snprintf(buf, sizeof(buf), "Time: %02d:%02d", gps.time.hour(), gps.time.minute());
                    display.drawStr(0, 42, buf);
                }
                display.drawStr(0, 52, "LIVE GPS");
            } else if (hasLastKnownGPS) {
                // Parse and display last known GPS data
                float lat, lon;
                int sat;
                if (sscanf(lastKnownGPSData, "%f,%f,%d", &lat, &lon, &sat) == 3) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "Lat: %.6f", lat);
                    display.drawStr(0, 12, buf);
                    snprintf(buf, sizeof(buf), "Lon: %.6f", lon);
                    display.drawStr(0, 22, buf);
                    snprintf(buf, sizeof(buf), "Sat: %d", sat);
                    display.drawStr(0, 32, buf);
                    display.drawStr(0, 42, "LAST KNOWN GPS");
                } else {
                    display.drawStr(0, 12, "Parse Error:");
                    display.drawStr(0, 22, lastKnownGPSData);
                    display.drawStr(0, 32, "GPS NO FIX");
                }
            } else {
                display.drawStr(0, 12, "No saved GPS data");
                display.drawStr(0, 22, "GPS NO FIX");
            }
            if (gpsFixAvailable) {
                display.drawStr(0, 55, "A/B=Save D=Back");
            } else {
                display.drawStr(0, 55, "B=Back D=Back");
            }
            display.sendBuffer();
            break;
        case STATE_SETTINGS:
            showSettingsMenu();
            break;
        case STATE_WIFI_SCAN:
            if (wifiScanComplete && wifiNetworkCount > 0) {
                currentState = STATE_WIFI_SELECT;
                showWifiSelectScreen();
            } else {
                showWifiScanScreen();
            }
            break;
        case STATE_WIFI_SELECT:
            showWifiSelectScreen();
            break;
        case STATE_WIFI_PASSWORD:
            showWifiPasswordScreen();
            break;
        case STATE_FREQUENCY_SELECT:
            showFrequencySelectScreen();
            break;
            
        case STATE_RESET_CONFIRM:
            showResetConfirmScreen();
            break;
        case STATE_WAITING_ACK:
            display.clearBuffer();
            display.drawStr(0, 15, "Sending...");
            display.drawStr(0, 25, ("Retry: " + String(retryCount) + "/" + String(MAX_RETRIES)).c_str());
            display.drawStr(0, 40, "Waiting for ACK...");
            display.sendBuffer();
            break;
        case STATE_VIEW_MESSAGES:
            viewMessagesScreen();
            break;
        case STATE_MESSAGE_DETAIL:
            viewMessageDetail();
            break;
        case STATE_MESSAGE_OPTIONS:
            viewMessageOptions();
            break;
        case STATE_DELETE_CONFIRM:
            viewDeleteConfirm();
            break;
        case STATE_FORWARD_MESSAGE:
            viewForwardMessage();
            break;
        case STATE_VIEW_GPS_LOCATION:
            viewGPSLocation();
            break;
        case STATE_SOS_EMERGENCY:
            showSOSScreen();
            break;
        default:
            showMenu();
            break;
    }
}

void viewMessagesScreen() {
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB08_tr);
    
    // Header - properly sized with exact width calculation
    String header = getMessageTypeName(currentMessageType);
    
    // Calculate exact width to prevent overflow
    int headerWidth = header.length() * 6; // Approximate width per character
    if (headerWidth > 80) {
        header = header.substring(0, 10) + "..";
    }
    
    display.drawStr(0, 10, header.c_str());
    
    // Show filter indicator with proper spacing
    char filterInd = 'A';
    switch(currentMessageType) {
        case 0: filterInd = 'A'; break; // All
        case 1: filterInd = 'R'; break; // Received
        case 2: filterInd = 'S'; break; // Sent
    }
    
    // Position filter indicator correctly
    display.drawStr(115, 10, "[ ]");
    display.drawStr(120, 10, String(filterInd).c_str());
    
    display.drawLine(0, 12, 128, 12);
    
    if (totalMessages == 0) {
        display.drawStr(0, 30, "No messages");
        display.drawStr(0, 40, "Press C to filter");
    } else {
        // Message list with precise spacing
        for (int i = 0; i < min(MESSAGES_PER_PAGE, totalMessages - viewOffset); i++) {
            int idx = viewOffset + i;
            int yPos = 20 + (i * 10); // Reduced spacing
            
            // Message indicator - more intuitive symbols
            char indicator = '?';
            switch(messageList[idx].type) {
                case 'R': indicator = '◀'; break; // Received (incoming)
                case 'S': indicator = '▶'; break; // Sent (outgoing)
                case 'A': indicator = '✓'; break; // Ack
                case 'F': indicator = '✗'; break; // Failed
            }
            
            // Truncate preview to fit exactly in available space
            char displayPreview[17];
            strncpy(displayPreview, messageList[idx].preview, 15);
            displayPreview[15] = '\0';
            if (strlen(messageList[idx].preview) > 15) {
                displayPreview[14] = '.';
                displayPreview[15] = '.';
                displayPreview[16] = '\0';
            }
            
            // Add GPS indicator if message has GPS data
            char line[20];
            if (messageList[idx].hasGPS) {
                snprintf(line, sizeof(line), "%c %s %c", indicator, displayPreview, '⌖');
            } else {
                snprintf(line, sizeof(line), "%c %s", indicator, displayPreview);
            }
            
            if (idx == messageIndex) {
                // Selection bar that doesn't overlap footer
                if (yPos + 8 < 52) {
                    display.drawBox(0, yPos - 1, 128, 10);
                    display.setDrawColor(0);
                    display.drawStr(2, yPos + 7, line);
                    display.setDrawColor(1);
                } else {
                    display.drawStr(2, yPos + 7, line);
                }
            } else {
                display.drawStr(2, yPos + 7, line);
            }
        }
        
        // Scroll indicators with precise positioning
        if (viewOffset > 0) {
            display.drawStr(122, 15, "↑");
        }
        if (viewOffset + MESSAGES_PER_PAGE < totalMessages) {
            display.drawStr(122, 50, "↓");
        }
        
        // Page info with exact positioning
        char pageInfo[16];
        snprintf(pageInfo, sizeof(pageInfo), "%d/%d", min(messageIndex + 1, totalMessages), totalMessages);
        
        // Calculate page info position to avoid overlap
        int pageInfoX = 128 - (strlen(pageInfo) * 6);
        if (pageInfoX < 100) pageInfoX = 100;
        display.drawStr(pageInfoX, 10, pageInfo);
    }
    
    // Footer - precise sizing
    display.drawLine(0, 52, 128, 52);
    
    // Footer text that fits exactly
    if (totalMessages > 0) {
        display.drawStr(2, 62, "5:Select 2/8:Nav C:Filter");
    } else {
        display.drawStr(2, 62, "C:Filter B:Back");
    }
    
    display.sendBuffer();
}

void viewMessageDetail() {
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB08_tr);
    
    if (messageIndex >= 0 && messageIndex < totalMessages) {
        File logFile = SD.open("/log.txt", FILE_READ);
        if (logFile) {
            logFile.seek(messageList[messageIndex].position);
            char lineBuffer[256];
            logFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1);
            logFile.close();
            
            // Header with message type and direction
            char header[20];
            char direction[5];
            switch(messageList[messageIndex].type) {
                case 'R': 
                    strcpy(header, "Received Message");
                    strcpy(direction, "◀ IN");
                    break;
                case 'S': 
                    strcpy(header, "Sent Message");
                    strcpy(direction, "OUT ▶");
                    break;
                case 'A': 
                    strcpy(header, "Acknowledgment");
                    strcpy(direction, "✓ ACK");
                    break;
                case 'F': 
                    strcpy(header, "Failed Message");
                    strcpy(direction, "✗ FAIL");
                    break;
                default: 
                    strcpy(header, "Message");
                    strcpy(direction, "");
            }
            
            display.drawStr(0, 10, header);
            display.drawStr(100, 10, direction);
            display.drawLine(0, 12, 128, 12);
            
            // Parse and display message
            char* timestampEnd = strchr(lineBuffer, ' ');
            char* contentStart = strchr(lineBuffer, ':');
            
            // Show timestamp if available
            if (timestampEnd && contentStart && timestampEnd < contentStart) {
                *timestampEnd = '\0';
                // Truncate timestamp if too long
                if (strlen(lineBuffer) > 16) {
                    char shortTime[17];
                    strncpy(shortTime, lineBuffer, 16);
                    shortTime[16] = '\0';
                    display.drawStr(0, 22, shortTime);
                } else {
                    display.drawStr(0, 22, lineBuffer);
                }
                contentStart = timestampEnd + 1;
            }
            
            if (contentStart) {
                contentStart++; // Skip first colon
                
                // Skip message ID if present
                if (strstr(lineBuffer, "MSG:") != NULL) {
                    char* msgContent = strchr(contentStart, ':');
                    if (msgContent) contentStart = msgContent + 1;
                }
                
                // Check for GPS data
                char* gpsSeparator = strchr(contentStart, '|');
                char messageContent[128] = "";
                char gpsData[64] = "";
                
                if (gpsSeparator) {
                    // Extract message and GPS data separately
                    size_t msgLength = gpsSeparator - contentStart;
                    strncpy(messageContent, contentStart, msgLength);
                    messageContent[msgLength] = '\0';
                    strncpy(gpsData, gpsSeparator + 1, sizeof(gpsData) - 1);
                    gpsData[sizeof(gpsData) - 1] = '\0';
                } else {
                    strncpy(messageContent, contentStart, sizeof(messageContent) - 1);
                    messageContent[sizeof(messageContent) - 1] = '\0';
                }
                
                // Word wrap the message content
                int yPos = 34; // Start below header and timestamp
                char* wordStart = messageContent;
                char line[21] = "";
                
                while (*wordStart && yPos <= 48) { // Stop before footer
                    char* spacePos = strchr(wordStart, ' ');
                    if (!spacePos) spacePos = wordStart + strlen(wordStart);
                    
                    int wordLen = spacePos - wordStart;
                    int lineLen = strlen(line);
                    
                    if (lineLen + wordLen + 1 > 20) {
                        display.drawStr(0, yPos, line);
                        yPos += 9; // Reduced line spacing
                        line[0] = '\0';
                        if (yPos > 48) break;
                    }
                    
                    if (line[0] != '\0') strcat(line, " ");
                    strncat(line, wordStart, wordLen);
                    line[20] = '\0';
                    
                    wordStart = spacePos;
                    if (*wordStart == ' ') wordStart++;
                }
                
                if (line[0] != '\0' && yPos <= 48) {
                    display.drawStr(0, yPos, line);
                }
                
                // Show GPS indicator if message has GPS data
                if (gpsData[0] != '\0') {
                    display.drawStr(110, 22, "⌖");
                }
            }
        }
    }
    
    // Footer with clear options
    display.drawLine(0, 52, 128, 52);
    if (messageList[messageIndex].hasGPS) {
        display.drawStr(2, 62, "Back:B  Reply:5  GPS:A");
    } else {
        display.drawStr(2, 62, "Back:B  Reply:5  Opt:A");
    }
    
    display.sendBuffer();
}

void viewGPSLocation() {
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB08_tr);
    
    if (messageIndex >= 0 && messageIndex < totalMessages) {
        File logFile = SD.open("/log.txt", FILE_READ);
        if (logFile) {
            logFile.seek(messageList[messageIndex].position);
            char lineBuffer[256];
            logFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1);
            logFile.close();
            
            display.drawStr(0, 10, "GPS Location:");
            display.drawLine(0, 12, 128, 12);
            
            // Extract GPS data from the message
            char* contentStart = strchr(lineBuffer, ':');
            if (contentStart) {
                contentStart++;
                if (strstr(lineBuffer, "MSG:") != NULL) {
                    char* msgContent = strchr(contentStart, ':');
                    if (msgContent) contentStart = msgContent + 1;
                }
                
                // Find GPS data
                char* gpsSeparator = strchr(contentStart, '|');
                if (gpsSeparator) {
                    // Parse GPS data (format: lat,lon,satellites)
                    float lat, lon;
                    int satellites;
                    if (sscanf(gpsSeparator + 1, "%f,%f,%d", &lat, &lon, &satellites) == 3) {
                        char buf[32];
                        
                        // Display latitude
                        snprintf(buf, sizeof(buf), "Lat: %.6f", lat);
                        display.drawStr(0, 24, buf);
                        
                        // Display longitude
                        snprintf(buf, sizeof(buf), "Lon: %.6f", lon);
                        display.drawStr(0, 34, buf);
                        
                        // Display satellite count
                        snprintf(buf, sizeof(buf), "Sat: %d", satellites);
                        display.drawStr(0, 44, buf);
                    } else {
                        display.drawStr(0, 24, "Invalid GPS data");
                    }
                } else {
                    display.drawStr(0, 24, "No GPS data found");
                }
            }
        }
    }
    
    // Footer with clear options
    display.drawLine(0, 52, 128, 52);
    display.drawStr(2, 62, "B:Back");
    
    display.sendBuffer();
}

void viewMessageOptions() {
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB08_tr);
    
    // Header with message type indicator
    char header[20];
    if (messageIndex >= 0 && messageIndex < totalMessages) {
        switch(messageList[messageIndex].type) {
            case 'R': strcpy(header, "Options [Received]"); break;
            case 'S': strcpy(header, "Options [Sent]"); break;
            case 'A': strcpy(header, "Options [Ack]"); break;
            case 'F': strcpy(header, "Options [Failed]"); break;
            default: strcpy(header, "Message Options");
        }
    } else {
        strcpy(header, "Message Options");
    }
    display.drawStr(0, 10, header);
    display.drawLine(0, 12, 128, 12);
    
    // Options list with numbers
    display.drawStr(0, 24, "1. Reply to message");
    display.drawStr(0, 34, "2. Delete message");
    display.drawStr(0, 44, "3. Forward message");
    
    // Footer with clear instructions
    display.drawLine(0, 52, 128, 52);
    display.drawStr(2, 62, "1-3:Select  B:Back");
    
    display.sendBuffer();
}

void viewDeleteConfirm() {
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB08_tr);
    
    display.drawStr(0, 10, "Confirm Deletion");
    display.drawLine(0, 12, 128, 12);
    
    // Show message preview being deleted
    if (messageIndex >= 0 && messageIndex < totalMessages) {
        char preview[18];
        strncpy(preview, messageList[messageIndex].preview, 16);
        preview[16] = '\0';
        if (strlen(messageList[messageIndex].preview) > 16) {
            preview[15] = '.';
            preview[16] = '.';
            preview[17] = '\0';
        }
        
        display.drawStr(0, 25, "Delete this message:");
        display.drawStr(0, 35, preview);
    } else {
        display.drawStr(0, 25, "Delete this message?");
    }
    
    // Footer with clear options
    display.drawLine(0, 52, 128, 52);
    display.drawStr(2, 62, "Confirm:5  Cancel:B");
    
    display.sendBuffer();
}

void viewForwardMessage() {
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB08_tr);
    
    display.drawStr(0, 10, "Forward Message:");
    display.drawStr(0, 22, forwardMessage);
    display.drawStr(0, 40, "A=Send B=Cancel C=Del");
    
    display.sendBuffer();
}

void showWifiSetupScreen() {
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB08_tr);
    
    if (wifiSetupStep == 0) {
        display.drawStr(0, 10, "WiFi Setup: SSID");
        display.drawStr(0, 25, wifiInputBuffer);
        display.drawStr(0, 40, "Press # to continue");
        display.drawStr(0, 55, "Press * for backspace");
    } else {
        display.drawStr(0, 10, "WiFi Setup: Password");
        display.drawStr(0, 25, "****************");
        display.drawStr(0, 40, "Press # to save");
        display.drawStr(0, 55, "Press * for backspace");
    }
    
    display.sendBuffer();
}

// --- EEPROM Functions ---
void saveWifiCredentials() {
    EEPROM.begin(512);
    int addr = 0;
    
    // Write SSID
    EEPROM.write(addr++, wifiSSID[0] != '\0' ? 1 : 0);
    for (int i = 0; i < 32; i++) {
        EEPROM.write(addr++, wifiSSID[i]);
        if (wifiSSID[i] == '\0') break;
    }
    
    // Write Password
    EEPROM.write(addr++, wifiPassword[0] != '\0' ? 1 : 0);
    for (int i = 0; i < 64; i++) {
        EEPROM.write(addr++, wifiPassword[i]);
        if (wifiPassword[i] == '\0') break;
    }
    
    EEPROM.commit();
    EEPROM.end();
}

void loadWifiCredentials() {
    EEPROM.begin(512);
    int addr = 0;
    
    // Read SSID
    if (EEPROM.read(addr++) == 1) {
        for (int i = 0; i < 32; i++) {
            wifiSSID[i] = EEPROM.read(addr++);
            if (wifiSSID[i] == '\0') break;
        }
    } else {
        wifiSSID[0] = '\0';
        addr += 32;
    }
    
    // Read Password
    if (EEPROM.read(addr++) == 1) {
        for (int i = 0; i < 64; i++) {
            wifiPassword[i] = EEPROM.read(addr++);
            if (wifiPassword[i] == '\0') break;
        }
    } else {
        wifiPassword[0] = '\0';
    }
    
    EEPROM.end();
    
    wifiConfigured = (wifiSSID[0] != '\0' && wifiPassword[0] != '\0');
}

// --- Web Server Functions ---
void setupWebServer() {
    server.on("/", handleRoot);
    server.on("/send", HTTP_POST, handleSendMessage);
    server.on("/messages", HTTP_GET, handleGetMessages);
    server.on("/status", HTTP_GET, handleGetStatus);
    server.on("/frequency", HTTP_POST, handleSetFrequency);
    server.on("/frequency", HTTP_GET, handleGetFrequency);
    server.on("/sos/start", HTTP_POST, handleStartSOS);
    server.on("/sos/stop", HTTP_POST, handleStopSOS);
    server.on("/sos/status", HTTP_GET, handleSOSStatus);
    
    server.begin();
    // HTTP server started
}

void handleRoot() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Offline Communication Mesh</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; margin: 0; padding: 20px; background: #f0f0f0; }
        .container { max-width: 800px; margin: auto; background: white; padding: 20px; border-radius: 10px; }
        h1 { text-align: center; color: #2c3e50; }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; font-weight: bold; }
        input[type=text], textarea { width: 100%; padding: 10px; font-size: 16px; border: 1px solid #ddd; border-radius: 5px; }
        button { background: #3498db; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; }
        button:hover { background: #2980b9; }
        .message-list { margin-top: 20px; border-top: 1px solid #ddd; padding-top: 20px; }
        .message { padding: 10px; border-bottom: 1px solid #eee; }
        .message:last-child { border-bottom: none; }
        .status { margin-top: 20px; padding: 15px; background: #ecf0f1; border-radius: 5px; }
        .message { margin: 10px 0; padding: 10px; background: #f8f9fa; border-left: 4px solid #3498db; }
        .gps-status { color: #27ae60; font-weight: bold; }
        .no-gps { color: #e74c3c; }
        .sos-section { margin: 20px 0; padding: 20px; background: #fff3cd; border: 2px solid #ffc107; border-radius: 10px; }
        .sos-active { background: #f8d7da; border-color: #dc3545; }
        .sos-button { padding: 12px 24px; font-size: 16px; font-weight: bold; border: none; border-radius: 5px; cursor: pointer; margin: 5px; }
        .sos-start { background: #dc3545; color: white; }
        .sos-stop { background: #6c757d; color: white; }
        .sos-status { font-weight: bold; margin: 10px 0; }
        .blinking { animation: blink 1s infinite; }
        @keyframes blink { 0%, 50% { opacity: 1; } 51%, 100% { opacity: 0.3; } }
    </style>
</head>
<body>
    <div class="container">
        <h1>Offline Communication Mesh</h1>
        
        <!-- SOS Emergency Section -->
        <div id="sosSection" class="sos-section">
            <h2>SOS Emergency</h2>
            <div id="sosStatus" class="sos-status">SOS Status: Inactive</div>
            <button id="sosStartBtn" class="sos-button sos-start" onclick="startSOS()">START SOS</button>
            <button id="sosStopBtn" class="sos-button sos-stop" onclick="stopSOS()" style="display:none;">STOP SOS</button>
            <div id="sosInfo" style="margin-top: 10px; font-size: 14px;">
                Emergency broadcast will send help message with location data every 10 seconds
            </div>
        </div>
        
        <div class="form-group">
            <label for="message">Send Message:</label>
            <textarea id="message" rows="3" placeholder="Type your message here..."></textarea>
        </div>
        
        <div class="form-group">
            <label>
                <input type="checkbox" id="includeGPS"> Include GPS location
            </label>
        </div>
        
        <button onclick="sendMessage()">Send Message</button>
        
        <div id="messageStatus" style="margin-top: 10px;"></div>
        
        <div class="status">
            <h3>Device Status</h3>
            <div id="statusInfo">Loading...</div>
        </div>
        
        <div class="status">
            <h3>LoRa Frequency Settings</h3>
            <div class="form-group">
                <label for="frequency">Select LoRa Frequency:</label>
                <select id="frequency" style="width: 100%; padding: 10px; font-size: 16px; border: 1px solid #ddd; border-radius: 5px;">
                    <option value="0">433 MHz</option>
                    <option value="1">169 MHz</option>
                </select>
            </div>
            <button onclick="setFrequency()">Update Frequency</button>
            <div id="frequencyStatus" style="margin-top: 10px;"></div>
        </div>
        
        <div class="message-list">
            <h3>Recent Messages</h3>
            <div id="messages">Loading messages...</div>
        </div>
    </div>

    <script>
        function sendMessage() {
            const message = document.getElementById('message').value;
            const includeGPS = document.getElementById('includeGPS').checked;
            
            if (!message) {
                showStatus('Please enter a message', 'error');
                return;
            }
            
            showStatus('Sending message...', '');
            
            fetch('/send', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({
                    message: message,
                    gps: includeGPS
                })
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    showStatus('Message sent successfully!', 'success');
                    document.getElementById('message').value = '';
                    loadMessages();
                    updateStatus();
                } else {
                    showStatus('Error: ' + data.error, 'error');
                }
            })
            .catch(error => {
                showStatus('Error sending message', 'error');
                console.error('Error:', error);
            });
        }
        
        function loadMessages() {
            fetch('/messages')
            .then(response => response.json())
            .then(data => {
                console.log('Received data:', data);
                const messagesDiv = document.getElementById('messages');
                if (data.messages && data.messages.length > 0) {
                    let html = '';
                    data.messages.forEach(msg => {
                        console.log('Processing message:', msg);
                        html += `<div class="message"><strong>${msg.time}</strong> ${msg.sender}: ${msg.text}`;
                        if (msg.gps) html += ' <span style="color:#27ae60;">[GPS]</span>';
                        html += '</div>';
                    });
                    messagesDiv.innerHTML = html;
                } else {
                    console.log('No messages found in data');
                    messagesDiv.innerHTML = 'No messages yet';
                }
            })
            .catch(error => {
                console.error('Error loading messages:', error);
                document.getElementById('messages').innerHTML = 'Error loading messages';
            });
        }
        
        function startSOS() {
            fetch('/sos/start', { method: 'POST' })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    updateSOSStatus();
                    showStatus('SOS ACTIVATED!', 'error');
                }
            })
            .catch(error => {
                console.error('Error starting SOS:', error);
                showStatus('Error starting SOS', 'error');
            });
        }
        
        function stopSOS() {
            fetch('/sos/stop', { method: 'POST' })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    updateSOSStatus();
                    showStatus('SOS STOPPED', 'success');
                }
            })
            .catch(error => {
                console.error('Error stopping SOS:', error);
                showStatus('Error stopping SOS', 'error');
            });
        }
        
        function updateSOSStatus() {
            fetch('/sos/status')
            .then(response => response.json())
            .then(data => {
                const sosSection = document.getElementById('sosSection');
                const sosStatus = document.getElementById('sosStatus');
                const sosStartBtn = document.getElementById('sosStartBtn');
                const sosStopBtn = document.getElementById('sosStopBtn');
                const sosInfo = document.getElementById('sosInfo');
                
                if (data.active) {
                    sosSection.classList.add('sos-active');
                    sosStatus.innerHTML = `<span class="blinking">🚨 SOS ACTIVE - ${data.count} messages sent</span>`;
                    sosStartBtn.style.display = 'none';
                    sosStopBtn.style.display = 'inline-block';
                    
                    let locationStatus = '';
                    if (data.gpsFix) {
                        locationStatus = 'Current GPS location';
                    } else if (data.hasLastKnownGPS) {
                        locationStatus = 'Last known GPS location';
                    } else {
                        locationStatus = 'No GPS location available';
                    }
                    sosInfo.innerHTML = `Broadcasting emergency message every 10 seconds with ${locationStatus}`;
                } else {
                    sosSection.classList.remove('sos-active');
                    sosStatus.innerHTML = 'SOS Status: Inactive';
                    sosStartBtn.style.display = 'inline-block';
                    sosStopBtn.style.display = 'none';
                    sosInfo.innerHTML = 'Emergency broadcast will send help message with location data every 10 seconds';
                }
            })
            .catch(error => {
                console.error('Error fetching SOS status:', error);
            });
        }

        function updateStatus() {
            fetch('/status')
            .then(response => response.json())
            .then(data => {
                const statusDiv = document.getElementById('statusInfo');
                statusDiv.innerHTML = `
                    <strong>WiFi:</strong> ${data.wifi ? 'Connected' : 'Disconnected'}<br>
                    <strong>LoRa:</strong> ${data.lora ? 'Active' : 'Inactive'}<br>
                    <strong>GPS:</strong> <span class="${data.gps ? 'gps-status' : 'no-gps'}">${data.gps ? 'Fix Available' : 'No Fix'}</span><br>
                    <strong>Messages:</strong> ${data.messageCount}<br>
                    <strong>Uptime:</strong> ${Math.floor(data.uptime / 1000)} seconds
                `;
            })
            .catch(error => {
                document.getElementById('statusInfo').innerHTML = 'Error loading status';
            });
            
            // Load current frequency
            fetch('/frequency')
            .then(response => response.json())
            .then(data => {
                document.getElementById('frequency').value = data.frequency;
            })
            .catch(error => {
                console.error('Error loading frequency:', error);
            });
        }
        
        function setFrequency() {
            const frequency = document.getElementById('frequency').value;
            
            fetch('/frequency', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({
                    frequency: parseInt(frequency)
                })
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    document.getElementById('frequencyStatus').innerHTML = 
                        '<span style="color: green;">Frequency updated successfully!</span>';
                    updateStatus();
                } else {
                    document.getElementById('frequencyStatus').innerHTML = 
                        '<span style="color: red;">Error: ' + data.error + '</span>';
                }
            })
            .catch(error => {
                document.getElementById('frequencyStatus').innerHTML = 
                    '<span style="color: red;">Error updating frequency</span>';
                console.error('Error:', error);
            });
        }
        
        function showStatus(text, className) {
            const statusDiv = document.getElementById('messageStatus');
            statusDiv.textContent = text;
            statusDiv.className = className;
            
            if (className === 'success') {
                setTimeout(() => {
                    statusDiv.textContent = '';
                    statusDiv.className = '';
                    
                    // Load initial data
                    loadMessages();
                    updateStatus();
                    updateSOSStatus();
                    
                    // Auto-refresh every 5 seconds
                    setInterval(() => {
                        loadMessages();
                        updateStatus();
                        updateSOSStatus();
                    }, 5000);
                }, 3000);
            }
        }
        
        // Load initial data
        loadMessages();
        updateStatus();
        updateSOSStatus();
        
        // Auto-refresh every 5 seconds
        setInterval(() => {
            loadMessages();
            updateStatus();
            updateSOSStatus();
        }, 5000);
    </script>
</body>
</html>
)rawliteral";

    server.send(200, "text/html", html);
}

void handleSendMessage() {
    if (server.hasArg("plain")) {
        String message = server.arg("plain");
        
        // Parse JSON
        int messageStart = message.indexOf("\"message\":\"") + 11;
        int messageEnd = message.indexOf("\"", messageStart);
        String msgText = message.substring(messageStart, messageEnd);
        
        int gpsStart = message.indexOf("\"gps\":") + 6;
        int gpsEnd = message.indexOf(",", gpsStart);
        if (gpsEnd == -1) gpsEnd = message.indexOf("}", gpsStart);
        bool includeGps = message.substring(gpsStart, gpsEnd) == "true";
        
        // Send the message via LoRa
        generateMessageId(pendingMessageId, sizeof(pendingMessageId));
        safeStringCopy(pendingMessage, msgText.c_str(), sizeof(pendingMessage));
        includeGPS = includeGps;
        retryCount = 0;
        currentState = STATE_WAITING_ACK;
        lastSendTime = 0; // Force immediate send
        executeState();
        
        // Log to SD card with timestamp
        File logFile = SD.open("/log.txt", FILE_APPEND);
        if (logFile) {
            String timestamp = String(millis());
            logFile.println(timestamp + " SENT: " + msgText);
            logFile.close();
        }
        
        server.send(200, "application/json", "{\"success\":true}");
    } else {
        server.send(400, "application/json", "{\"success\":false, \"error\":\"No message provided\"}");
    }
}

void handleGetMessages() {
    server.send(200, "application/json", getMessagesJSON());
}

void handleGetStatus() {
    String statusJSON = "{";
    
    // Check WiFi status and provide appropriate IP info
    if (WiFi.status() == WL_CONNECTED) {
        statusJSON += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
        statusJSON += "\"wifiStatus\":\"Connected\",";
    } else {
        statusJSON += "\"ip\":\"Not Connected\",";
        statusJSON += "\"wifiStatus\":\"Disconnected\",";
    }
    
    statusJSON += "\"gpsFix\":" + String(gps.location.isValid() ? "true" : "false") + ",";
    statusJSON += "\"lat\":" + String(gps.location.isValid() ? String(gps.location.lat(), 6) : "0") + ",";
    statusJSON += "\"lon\":" + String(gps.location.isValid() ? String(gps.location.lng(), 6) : "0") + ",";
    statusJSON += "\"receivedCount\":" + String(receivedCount) + ",";
    statusJSON += "\"sentCount\":" + String(sentCount) + ",";
    statusJSON += "\"loraStatus\":\"" + String(loraInitialized ? "Connected" : "Disconnected") + "\",";
    statusJSON += "\"sosActive\":" + String(sosActive ? "true" : "false") + ",";
    statusJSON += "\"sosCount\":" + String(sosCounter);
    statusJSON += "}";
    
    server.send(200, "application/json", statusJSON);
}

void handleStartSOS() {
    startSOS();
    server.send(200, "application/json", "{\"success\":true,\"message\":\"SOS activated\"}");
}

void handleStopSOS() {
    stopSOS();
    server.send(200, "application/json", "{\"success\":true,\"message\":\"SOS deactivated\"}");
}

void handleSOSStatus() {
    String sosJSON = "{";
    sosJSON += "\"active\":" + String(sosActive ? "true" : "false") + ",";
    sosJSON += "\"count\":" + String(sosCounter) + ",";
    sosJSON += "\"gpsFix\":" + String(gps.location.isValid() ? "true" : "false") + ",";
    sosJSON += "\"hasLastKnownGPS\":" + String(hasLastKnownGPS ? "true" : "false");
    sosJSON += "}";
    
    server.send(200, "application/json", sosJSON);
}

void handleSetFrequency() {
    if (server.hasArg("plain")) {
        String body = server.arg("plain");
        
        // Parse JSON manually (simple approach)
        int freqStart = body.indexOf("frequency") + 11;
        int freqEnd = body.indexOf("}", freqStart);
        if (freqEnd == -1) freqEnd = body.length();
        
        String freqStr = body.substring(freqStart, freqEnd);
        freqStr.trim();
        
        int newFreq = freqStr.toInt();
        
        if (newFreq == 0 || newFreq == 1) {
            loraFrequency = newFreq;
            saveFrequencySettings();
            setLoRaFrequency();
            
            String response = "{\"success\": true, \"frequency\": " + String(loraFrequency) + "}";
            server.send(200, "application/json", response);
        } else {
            server.send(400, "application/json", "{\"success\": false, \"error\": \"Invalid frequency\"}");
        }
    } else {
        server.send(400, "application/json", "{\"success\": false, \"error\": \"No data received\"}");
    }
}

void handleGetFrequency() {
    String json = "{\"frequency\": " + String(loraFrequency) + ", \"name\": \"" + String(freqNames[loraFrequency]) + "\"}";
    server.send(200, "application/json", json);
}

String getMessagesJSON() {
    loadMessageList();
    
    String json = "{\"messages\":[";
    bool first = true;
    
    // Return empty array if no messages
    if (totalMessages == 0) {
        json += "]}";
        return json;
    }
    
    for (int i = 0; i < totalMessages; i++) {
        File logFile = SD.open("/log.txt", FILE_READ);
        if (logFile) {
            logFile.seek(messageList[i].position);
            char lineBuffer[256];
            int bytesRead = logFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1);
            lineBuffer[bytesRead] = '\0'; // Null terminate properly
            logFile.close();
            
            // Parse the log line
            char* timeStart = lineBuffer;
            char* typeStart = strchr(lineBuffer, ' ');
            if (typeStart) {
                *typeStart = '\0';
                typeStart++;
                
                char* contentStart = strchr(typeStart, ':');
                if (contentStart) {
                    *contentStart = '\0';
                    contentStart += 2; // Skip ": "
                    
                    if (!first) json += ",";
                    first = false;
                    
                    json += "{\"time\":\"";
                    json += timeStart;
                    json += "\",\"sender\":\"";
                    json += (strncmp(typeStart, "RCVD", 4) == 0) ? "Received" : "Sent";
                    json += "\",\"text\":\"";
                    
                    // Extract message text (before GPS data if present)
                    char* gpsSeparator = strchr(contentStart, '|');
                    if (gpsSeparator) {
                        *gpsSeparator = '\0';
                    }
                    
                    // Clean and escape message text
                    String messageText = String(contentStart);
                    // Remove any non-printable characters
                    String cleanText = "";
                    for (int j = 0; j < messageText.length(); j++) {
                        char c = messageText.charAt(j);
                        if (c >= 32 && c <= 126) { // Only printable ASCII
                            cleanText += c;
                        }
                    }
                    // Escape JSON special characters
                    cleanText.replace("\\", "\\\\");
                    cleanText.replace("\"", "\\\"");
                    cleanText.replace("\n", "\\n");
                    cleanText.replace("\r", "\\r");
                    cleanText.replace("\t", "\\t");
                    
                    json += cleanText;
                    json += "\",\"gps\":";
                    json += (gpsSeparator != nullptr) ? "true" : "false";
                    json += "}";
                }
            }
        }
    }
    
    json += "]}";
    return json;
}

// --- WiFi Setup Functions ---
void handleWifiInput(char key) {
    if (key == '#') {
        if (wifiSetupStep == 0) {
            // Save SSID and move to password
            safeStringCopy(wifiSSID, wifiInputBuffer, sizeof(wifiSSID));
            wifiInputIndex = 0;
            wifiInputBuffer[0] = '\0';
            wifiSetupStep = 1;
        } else {
            // Save password and complete setup
            safeStringCopy(wifiPassword, wifiInputBuffer, sizeof(wifiPassword));
            wifiInputIndex = 0;
            wifiInputBuffer[0] = '\0';
            wifiSetupStep = 0;
            
            // Save to EEPROM
            saveWifiCredentials();
            
            // Attempt to connect to WiFi
            WiFi.begin(wifiSSID, wifiPassword);
            
            display.clearBuffer();
            display.drawStr(0, 15, "Connecting to WiFi...");
            display.drawStr(0, 30, wifiSSID);
            display.sendBuffer();
            
            int attempts = 0;
            while (WiFi.status() != WL_CONNECTED && attempts < 20) {
                delay(500);
                display.drawStr(0, 45, ".");
                display.sendBuffer();
                attempts++;
            }
            
            if (WiFi.status() == WL_CONNECTED) {
                display.clearBuffer();
                display.drawStr(0, 15, "WiFi Connected!");
                display.drawStr(0, 30, ("IP: " + WiFi.localIP().toString()).c_str());
                display.sendBuffer();
                delay(2000);
                
                // Start web server
                setupWebServer();
                wifiConfigured = true;
                currentState = STATE_MAIN_MENU;
            } else {
                display.clearBuffer();
                display.drawStr(0, 15, "Connection failed!");
                display.drawStr(0, 30, "Check credentials");
                display.sendBuffer();
                delay(2000);
                currentState = STATE_SETTINGS;
            }
        }
    } else if (key == '*') {
        if (wifiInputIndex > 0) {
            wifiInputIndex--;
            wifiInputBuffer[wifiInputIndex] = '\0';
        }
    } else if (wifiInputIndex < sizeof(wifiInputBuffer) - 1) {
        wifiInputBuffer[wifiInputIndex] = key;
        wifiInputIndex++;
        wifiInputBuffer[wifiInputIndex] = '\0';
    }
    
    showWifiSetupScreen();
}

// --- WiFi Scanning Functions ---
void scanWifiNetworks() {
    // Only scan if not already scanning or complete
    if (wifiScanComplete) return;
    
    display.clearBuffer();
    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(0, 15, "Scanning WiFi...");
    display.drawStr(0, 30, "Please wait...");
    display.sendBuffer();
    
    wifiNetworkCount = 0;
    selectedNetworkIndex = 0;
    
    // Ensure WiFi is properly initialized for scanning
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    delay(50);
    
    // Clear any previous scan results
    WiFi.scanDelete();
    
    int n = WiFi.scanNetworks(false, false);
    
    if (n >= 0) {
        if (n == 0) {
            wifiNetworkCount = 0;
        } else {
            wifiNetworkCount = min(n, 10); // Limit to 10 networks
            
            for (int i = 0; i < wifiNetworkCount; i++) {
                wifiNetworks[i] = WiFi.SSID(i);
            }
        }
        
        wifiScanComplete = true;
        if (wifiNetworkCount > 0) {
            currentState = STATE_WIFI_SELECT;
        }
    }
}

void showWifiScanScreen() {
    // Start scan if not started
    if (!wifiScanComplete) {
        scanWifiNetworks();
    }
    
    display.clearBuffer();
    display.setFont(u8g2_font_6x10_tr);
    
    if (!wifiScanComplete) {
        display.drawStr(0, 15, "Scanning WiFi...");
        display.drawStr(0, 30, "Please wait...");
        display.drawStr(0, 50, "B=Back");
    } else if (wifiNetworkCount == 0) {
        display.drawStr(0, 15, "No networks found");
        display.drawStr(0, 30, "Press B to go back");
    } else {
        // Networks found, show select screen instead
        showWifiSelectScreen();
        return;
    }
    
    display.sendBuffer();
}

void showWifiSelectScreen() {
    display.clearBuffer();
    display.setFont(u8g2_font_6x10_tr);
    
    display.drawStr(0, 10, "Select WiFi Network:");
    display.drawLine(0, 12, 128, 12);
    
    if (wifiNetworkCount > 0) {
        // Show up to 4 networks at a time
        int startIndex = max(0, selectedNetworkIndex - 1);
        int endIndex = min(wifiNetworkCount, startIndex + 4);
        
        for (int i = startIndex; i < endIndex; i++) {
            int yPos = 20 + ((i - startIndex) * 10);
            
            // Truncate network name if too long
            String networkName = wifiNetworks[i];
            if (networkName.length() > 18) {
                networkName = networkName.substring(0, 15) + "...";
            }
            
            if (i == selectedNetworkIndex) {
                display.drawBox(0, yPos - 1, 128, 10);
                display.setDrawColor(0);
                display.drawStr(2, yPos + 7, networkName.c_str());
                display.setDrawColor(1);
            } else {
                display.drawStr(2, yPos + 7, networkName.c_str());
            }
        }
        
        // Show scroll indicators
        if (selectedNetworkIndex > 0) {
            display.drawStr(122, 15, "↑");
        }
        if (selectedNetworkIndex < wifiNetworkCount - 1) {
            display.drawStr(122, 50, "↓");
        }
    }
    
    display.drawLine(0, 52, 128, 52);
    display.drawStr(2, 62, "2/8:Nav 5:Select B:Back");
    
    display.sendBuffer();
}

void showWifiPasswordScreen() {
    display.clearBuffer();
    display.setFont(u8g2_font_6x10_tr);
    
    display.drawStr(0, 10, "WiFi Password:");
    
    // Show selected network name (truncated if needed)
    String networkName = wifiNetworks[selectedNetworkIndex];
    if (networkName.length() > 20) {
        networkName = networkName.substring(0, 17) + "...";
    }
    display.drawStr(0, 22, networkName.c_str());
    
    // Show password as asterisks
    String passwordDisplay = "";
    for (int i = 0; i < strlen(wifiInputBuffer); i++) {
        passwordDisplay += "*";
    }
    display.drawStr(0, 34, passwordDisplay.c_str());
    
    // Show input mode and case
    char modeStr[25];
    if (wifiInputMode == 0) {
        snprintf(modeStr, sizeof(modeStr), "Mode: Numbers");
    } else {
        snprintf(modeStr, sizeof(modeStr), "Mode: %s", wifiUpperCase ? "UPPER" : "lower");
    }
    display.drawStr(0, 46, modeStr);
    
    display.drawLine(0, 52, 128, 52);
    display.drawStr(2, 62, "A:OK C:Del *:Mode #:Case B:Back");
    
    display.sendBuffer();
}

void handleWifiScanInput(char key) {
    if (key == 'B') {
        currentState = STATE_SETTINGS;
        executeState();
    } else if (wifiScanComplete && wifiNetworkCount > 0) {
        currentState = STATE_WIFI_SELECT;
        executeState();
    }
}

void handleWifiSelectInput(char key) {
    if (key == '2' && selectedNetworkIndex > 0) {
        selectedNetworkIndex--;
        executeState();
    } else if (key == '8' && selectedNetworkIndex < wifiNetworkCount - 1) {
        selectedNetworkIndex++;
        executeState();
    } else if (key == '5' && wifiNetworkCount > 0) {
        // Select network and go to password entry
        safeStringCopy(wifiSSID, wifiNetworks[selectedNetworkIndex].c_str(), sizeof(wifiSSID));
        wifiInputIndex = 0;
        wifiInputBuffer[0] = '\0';
        wifiInputMode = 0; // Start with numbers mode
        wifiLastKey = -1;
        wifiCurrentLetterIndex = 0;
        wifiLastPress = 0;
        wifiUpperCase = false; // Start with lowercase
        currentState = STATE_WIFI_PASSWORD;
        executeState();
    } else if (key == 'B') {
        currentState = STATE_SETTINGS;
        executeState();
    }
}

void handleWifiPasswordInput(char key) {
    if (key == 'A') {
        // Connect to WiFi
        safeStringCopy(wifiPassword, wifiInputBuffer, sizeof(wifiPassword));
        saveWifiCredentials();
        
        display.clearBuffer();
        display.drawStr(0, 15, "Connecting...");
        display.drawStr(0, 30, wifiSSID);
        display.sendBuffer();
        
        WiFi.begin(wifiSSID, wifiPassword);
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            attempts++;
            display.drawStr(0, 45, ".");
            display.sendBuffer();
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            display.clearBuffer();
            display.drawStr(0, 15, "WiFi Connected!");
            display.drawStr(0, 30, ("IP: " + WiFi.localIP().toString()).c_str());
            display.sendBuffer();
            delay(2000);
            
            setupWebServer();
            wifiConfigured = true;
            currentState = STATE_MAIN_MENU;
            showMenu();
        } else {
            display.clearBuffer();
            display.drawStr(0, 15, "Connection failed!");
            display.drawStr(0, 30, "Check password");
            display.sendBuffer();
            delay(2000);
            currentState = STATE_WIFI_SELECT;
            executeState();
        }
    } else if (key == 'B') {
        // Go back to network selection
        wifiInputIndex = 0;
        wifiInputBuffer[0] = '\0';
        currentState = STATE_WIFI_SELECT;
        executeState();
    } else {
        // Use the same input system as message writing
        handleWifiTextInput(key, millis());
    }
}

void handleWifiTextInput(char key, unsigned long now) {
    // Buffer overflow protection
    if (wifiInputIndex >= sizeof(wifiInputBuffer) - 2) {
        return;
    }
    
    // Toggle input mode with '*' key
    if (key == '*') {
        wifiInputMode = !wifiInputMode; // Toggle between numbers and letters
        executeState();
        return;
    }
    
    // Toggle case with '#' key (only in letters mode)
    if (key == '#' && wifiInputMode == 1) {
        wifiUpperCase = !wifiUpperCase; // Toggle between upper and lower case
        executeState();
        return;
    }
    
    if (wifiInputMode == 0) { // Numbers mode
        if (key >= '0' && key <= '9') {
            if (wifiLastKey != -1) {
                wifiInputIndex++;
            }
            wifiInputBuffer[wifiInputIndex] = key;
            wifiInputIndex++;
            wifiLastKey = -1;
            wifiLastPress = now;
        }
    } else { // Letters mode
        if (key >= '1' && key <= '9') {
            int k = key - '0';
            if (k > 0 && k < 10) {
                if (wifiLastKey != k || (now - wifiLastPress > LETTER_TIMEOUT)) {
                    if (wifiLastKey != -1) {
                        wifiInputIndex++;
                    }
                    wifiCurrentLetterIndex = 0;
                } else {
                    wifiCurrentLetterIndex = (wifiCurrentLetterIndex + 1) % strlen(keyMapLetters[k]);
                }
                
                char letter = keyMapLetters[k][wifiCurrentLetterIndex];
                
                // Apply case conversion
                if (wifiUpperCase && letter >= 'a' && letter <= 'z') {
                    letter = letter - 'a' + 'A'; // Convert to uppercase
                } else if (!wifiUpperCase && letter >= 'A' && letter <= 'Z') {
                    letter = letter - 'A' + 'a'; // Convert to lowercase
                }
                
                wifiInputBuffer[wifiInputIndex] = letter;
                wifiLastKey = k;
                wifiLastPress = now;
            }
        } else if (key == '0') {
            if (wifiLastKey != -1) {
                wifiInputIndex++;
            }
            wifiInputBuffer[wifiInputIndex] = ' ';
            wifiInputIndex++;
            wifiLastKey = -1;
            wifiLastPress = now;
        }
    }

    if (key == 'C' && wifiInputIndex > 0) {
        wifiInputIndex--;
        wifiInputBuffer[wifiInputIndex] = '\0';
        wifiLastKey = -1;
        wifiLastPress = now;
    }
    
    // Ensure the buffer is always null-terminated
    wifiInputBuffer[wifiInputIndex + (wifiLastKey != -1 ? 1 : 0)] = '\0';
    executeState();
}

// --- Keypad Handling ---
void handleKeypadInput(char key) {
    resetWatchdog();
    
    switch (currentState) {
        case STATE_MAIN_MENU:
            if (key == '2' && menuIndex > 0) menuIndex--;
            else if (key == '8' && menuIndex < MENU_COUNT - 1) menuIndex++;
            else if (key == '5') {
                switch (menuIndex) {
                    case 0: 
                        currentState = STATE_SEND_MESSAGE; 
                        memset(fullMessage, 0, sizeof(fullMessage));
                        msgIndex = 0;
                        inputMode = 0; // Reset to numbers mode
                        break;
                    case 1: 
                        currentState = STATE_VIEW_MESSAGES;
                        messageIndex = 0;
                        viewOffset = 0;
                        currentMessageType = 0;
                        loadMessageList();
                        break;
                    case 2: currentState = STATE_GPS_INFO; break;
                    case 3: currentState = STATE_SETTINGS; break;
                    case 4: 
                        currentState = STATE_SOS_EMERGENCY;
                        startSOS();
                        break;
                }
            }
            executeState();
            break;

        case STATE_SETTINGS:
            if (key == '2' && settingsIndex > 0) {
                settingsIndex--;
            } else if (key == '8' && settingsIndex < SETTINGS_COUNT - 1) {
                settingsIndex++;
            } else if (key == '5') {
                switch (settingsIndex) {
                    case 0: // WiFi Setup
                        wifiScanComplete = false;
                        wifiNetworkCount = 0;
                        selectedNetworkIndex = 0;
                        currentState = STATE_WIFI_SCAN;
                        break;
                    case 1: // LoRa Frequency
                        frequencyIndex = loraFrequency; // Set current frequency as selected
                        currentState = STATE_FREQUENCY_SELECT;
                        break;
                    case 2: // Reset Messages
                        currentState = STATE_RESET_CONFIRM;
                        break;
                    case 3: // WiFi Disconnect
                        WiFi.disconnect();
                        wifiConfigured = false;
                        display.clearBuffer();
                        display.setFont(u8g2_font_6x10_tr);
                        display.drawStr(0, 20, "WiFi Disconnected");
                        display.sendBuffer();
                        delay(800);
                        break;
                    case 4: // Device Info
                        display.clearBuffer();
                        display.setFont(u8g2_font_6x10_tr);
                        display.drawStr(0, 10, "Device Info");
                        display.drawLine(0, 12, 128, 12);
                        display.drawStr(0, 20, "ESP32 Comm Mesh");
                        display.drawStr(0, 28, "Version: 1.0");
                        
                        if (WiFi.status() == WL_CONNECTED) {
                            display.drawStr(0, 36, "WiFi: Connected");
                        } else {
                            display.drawStr(0, 36, "WiFi: Disconnected");
                        }
                        
                        char freqStr[32];
                        snprintf(freqStr, sizeof(freqStr), "LoRa: %s", freqNames[loraFrequency]);
                        display.drawStr(0, 44, freqStr);
                        
                        display.drawStr(0, 60, "B=Back");
                        display.sendBuffer();
                        delay(1000);
                        break;
                }
            } else if (key == 'B' || key == 'b' || key == 'D' || key == 'd') {
                currentState = STATE_MAIN_MENU;
            }
            executeState();
            break;
            
        case STATE_WIFI_SCAN:
            handleWifiScanInput(key);
            break;
            
        case STATE_WIFI_SELECT:
            handleWifiSelectInput(key);
            break;
            
        case STATE_WIFI_PASSWORD:
            handleWifiPasswordInput(key);
            break;
            
        case STATE_FREQUENCY_SELECT:
            handleFrequencySelectInput(key);
            break;
            
        case STATE_WEB_INTERFACE:
            if (key == 'B' || key == 'D') {
                currentState = STATE_MAIN_MENU;
                showMenu();
            }
            break;
            
        case STATE_VIEW_MESSAGES:
            if (key == '2' && messageIndex > 0) {
                messageIndex--;
                if (messageIndex < viewOffset) viewOffset = messageIndex;
                executeState();
            } else if (key == '8' && messageIndex < totalMessages - 1) {
                messageIndex++;
                if (messageIndex >= viewOffset + MESSAGES_PER_PAGE) viewOffset++;
                executeState();
            } else if (key == '5' && totalMessages > 0) {
                currentState = STATE_MESSAGE_DETAIL;
                executeState();
            } else if (key == 'C') {
                currentMessageType = (currentMessageType + 1) % 3;
                loadMessageList();
                messageIndex = 0;
                viewOffset = 0;
                executeState();
            } else if (key == 'B' || key == 'D') {
                currentState = STATE_MAIN_MENU;
                executeState();
            }
            break;
            
        case STATE_MESSAGE_DETAIL:
            if (key == 'B' || key == 'D') {
                currentState = STATE_VIEW_MESSAGES;
                executeState();
            } else if (key == '5') {
                // Reply
                currentState = STATE_SEND_MESSAGE;
                memset(fullMessage, 0, sizeof(fullMessage));
                msgIndex = 0;
                inputMode = 0; // Reset to numbers mode
                executeState();
            } else if (key == 'A') {
                if (messageList[messageIndex].hasGPS) {
                    // View GPS location
                    currentState = STATE_VIEW_GPS_LOCATION;
                } else {
                    // Show options
                    currentState = STATE_MESSAGE_OPTIONS;
                }
                executeState();
            }
            break;
            
        case STATE_VIEW_GPS_LOCATION:
            if (key == 'B' || key == 'D') {
                currentState = STATE_MESSAGE_DETAIL;
                executeState();
            }
            break;
            
        case STATE_MESSAGE_OPTIONS:
            if (key == 'B' || key == 'D') {
                currentState = STATE_MESSAGE_DETAIL;
                executeState();
            } else if (key == '1') {
                // Reply
                currentState = STATE_SEND_MESSAGE;
                memset(fullMessage, 0, sizeof(fullMessage));
                msgIndex = 0;
                inputMode = 0; // Reset to numbers mode
                executeState();
            } else if (key == '2') {
                // Delete - show confirmation
                currentState = STATE_DELETE_CONFIRM;
                executeState();
            } else if (key == '3') {
                // Forward - extract message and go to send screen
                if (messageIndex >= 0 && messageIndex < totalMessages) {
                    File logFile = SD.open("/log.txt", FILE_READ);
                    if (logFile) {
                        logFile.seek(messageList[messageIndex].position);
                        char lineBuffer[256];
                        logFile.readBytesUntil('\n', lineBuffer, sizeof(lineBuffer) - 1);
                        logFile.close();
                        
                        if (extractMessageContent(lineBuffer, forwardMessage, sizeof(forwardMessage))) {
                            currentState = STATE_FORWARD_MESSAGE;
                        } else {
                            // Fallback: use preview
                            safeStringCopy(forwardMessage, messageList[messageIndex].preview, sizeof(forwardMessage));
                            currentState = STATE_FORWARD_MESSAGE;
                        }
                    }
                }
                executeState();
            }
            break;
            
        case STATE_DELETE_CONFIRM:
            if (key == '5') {
                // Confirm delete
                if (deleteMessageFromSD(messageIndex)) {
                    display.clearBuffer();
                    display.drawStr(0, 15, "Message deleted");
                    display.drawStr(0, 30, "successfully!");
                    display.sendBuffer();
                    delay(1500);
                    
                    // Go back to message list
                    currentState = STATE_VIEW_MESSAGES;
                    loadMessageList();
                    if (messageIndex >= totalMessages) {
                        messageIndex = max(0, totalMessages - 1);
                    }
                } else {
                    display.clearBuffer();
                    display.drawStr(0, 15, "Delete failed!");
                    display.sendBuffer();
                    delay(1500);
                    currentState = STATE_MESSAGE_DETAIL;
                }
                executeState();
            } else if (key == 'B' || key == 'D') {
                // Cancel delete
                currentState = STATE_MESSAGE_OPTIONS;
                executeState();
            }
            break;
            
        case STATE_RESET_CONFIRM:
            if (key == '5') {
                // Confirm reset - delete all messages
                resetAllMessages();
                display.clearBuffer();
                display.drawStr(0, 15, "All messages");
                display.drawStr(0, 25, "deleted!");
                display.drawStr(0, 40, "Returning to menu...");
                display.sendBuffer();
                delay(2000);
                currentState = STATE_SETTINGS;
                executeState();
            } else if (key == 'B' || key == 'D') {
                // Cancel reset
                currentState = STATE_SETTINGS;
                executeState();
            }
            break;
            
        case STATE_FORWARD_MESSAGE:
            if (key == 'A') {
                // Send forwarded message
                generateMessageId(pendingMessageId, sizeof(pendingMessageId));
                safeStringCopy(pendingMessage, forwardMessage, sizeof(pendingMessage));
                retryCount = 0;
                currentState = STATE_WAITING_ACK;
                lastSendTime = 0;
                executeState();
            } else if (key == 'B' || key == 'D') {
                // Cancel forward
                currentState = STATE_MESSAGE_OPTIONS;
                executeState();
            } else if (key == 'C' && strlen(forwardMessage) > 0) {
                // Delete character
                int len = strlen(forwardMessage);
                if (len > 0) {
                    forwardMessage[len - 1] = '\0';
                    executeState();
                }
            }
            break;
            
        case STATE_SEND_MESSAGE:
            handleMessageInput(key, millis());
            break;

        case STATE_GPS_INFO:
            if (key == 'A' || key == 'B') {
                // Save current GPS location as default
                if (gpsFixAvailable) {
                    strcpy(lastKnownGPSData, currentGPSData);
                    hasLastKnownGPS = true;
                    saveGPSToSD();
                    display.clearBuffer();
                    display.drawStr(0, 15, "GPS Location");
                    display.drawStr(0, 25, "Saved!");
                    display.sendBuffer();
                    delay(1500);
                    executeState();
                } else if (key == 'B') {
                    // If no GPS fix and B pressed, go back to menu
                    currentState = STATE_MAIN_MENU;
                    showMenu();
                }
            } else if (key == 'D') {
                currentState = STATE_MAIN_MENU;
                showMenu();
            }
            break;
            
        case STATE_LORA_RX:
            if (key == 'B' || key == 'D') {
                currentState = STATE_MAIN_MENU;
                executeState();
            }
            break;
            
        case STATE_WAITING_ACK:
            if (key == 'B' || key == 'D') {
                currentState = STATE_MAIN_MENU;
                executeState();
            }
            break;
            
        case STATE_SOS_EMERGENCY:
            if (key == 'B' || key == 'D') {
                if (sosActive) {
                    stopSOS();
                }
                currentState = STATE_MAIN_MENU;
                executeState();
            } else if (!sosActive) {
                // Any other key starts SOS when not active
                startSOS();
                executeState();
            }
            break;
    }
}

void handleMessageInput(char key, unsigned long now) {
    // Buffer overflow protection
    if (msgIndex >= sizeof(fullMessage) - 2) {
        return;
    }
    
    // Toggle input mode with '*' key
    if (key == '*') {
        inputMode = !inputMode; // Toggle between numbers and letters
        executeState();
        return;
    }
    
    if (inputMode == 0) { // Numbers mode
        if (key >= '0' && key <= '9') {
            if (lastKey != -1) {
                msgIndex++;
            }
            fullMessage[msgIndex] = key;
            msgIndex++;
            lastKey = -1;
            lastPress = now;
        }
    } else { // Letters mode
        if (key >= '1' && key <= '9') {
            int k = key - '0';
            if (k > 0 && k < 10) {
                if (lastKey != k || (now - lastPress > LETTER_TIMEOUT)) {
                    if (lastKey != -1) {
                        msgIndex++;
                    }
                    currentLetterIndex = 0;
                } else {
                    currentLetterIndex = (currentLetterIndex + 1) % strlen(keyMapLetters[k]);
                }
                fullMessage[msgIndex] = keyMapLetters[k][currentLetterIndex];
                lastKey = k;
                lastPress = now;
            }
        } else if (key == '0') {
            if (lastKey != -1) {
                msgIndex++;
            }
            fullMessage[msgIndex] = ' ';
            msgIndex++;
            lastKey = -1;
            lastPress = now;
        }
    }

    if (key == 'C' && msgIndex > 0) {
        msgIndex--;
        fullMessage[msgIndex] = '\0';
        lastKey = -1;
        lastPress = now;

    } else if (key == 'A') {
        // 'A' key to send the message via LoRa with acknowledgment
        generateMessageId(pendingMessageId, sizeof(pendingMessageId));
        safeStringCopy(pendingMessage, fullMessage, sizeof(pendingMessage));
        retryCount = 0;
        currentState = STATE_WAITING_ACK;
        lastSendTime = 0; // Force immediate send
        executeState();

    } else if (key == 'B') {
        // 'B' key to go back without sending
        memset(fullMessage, 0, sizeof(fullMessage));
        msgIndex = 0;
        lastKey = -1;
        currentState = STATE_MAIN_MENU;
        showMenu();
    } else if (key == 'D') {
        // 'D' key to toggle GPS inclusion
        includeGPS = !includeGPS;
        executeState();
    }
    
    // Ensure the message is always null-terminated
    fullMessage[msgIndex + (lastKey != -1 ? 1 : 0)] = '\0';
    executeState();
}

// Show reset confirmation screen
void showResetConfirmScreen() {
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB08_tr);
    display.drawStr(0, 15, "Reset All Messages?");
    display.drawStr(0, 30, "This will delete ALL");
    display.drawStr(0, 40, "stored messages!");
    display.drawStr(0, 55, "5:Confirm  B:Cancel");
    display.sendBuffer();
}

// Reset all messages by deleting the log file
void resetAllMessages() {
    if (SD.exists("/log.txt")) {
        SD.remove("/log.txt");
        // All messages deleted from SD card
    }
    
    // Clear message list in memory
    totalMessages = 0;
    messageIndex = 0;
    viewOffset = 0;
    
    // Reset message list array
    for (int i = 0; i < MAX_MESSAGES; i++) {
        messageList[i].type = 'U';
        messageList[i].preview[0] = '\0';
        messageList[i].hasGPS = false;
        messageList[i].position = 0;
    }
    
    // Message list cleared from memory
}

// --- SOS Emergency Functions ---
void showSOSScreen() {
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB08_tr);
    
    display.drawStr(0, 10, "SOS EMERGENCY");
    display.drawLine(0, 12, 128, 12);
    
    if (sosActive) {
        display.drawStr(0, 24, "BROADCASTING SOS...");
        
        char countStr[20];
        snprintf(countStr, sizeof(countStr), "Sent: %d messages", sosCounter);
        display.drawStr(0, 36, countStr);
        
        // Show location status
        if (gpsFixAvailable) {
            display.drawStr(0, 48, "GPS: ACTIVE");
        } else if (hasLastKnownGPS) {
            display.drawStr(0, 48, "GPS: LAST KNOWN");
        } else {
            display.drawStr(0, 48, "GPS: NO LOCATION");
        }
        
        // Blinking indicator
        if ((millis() / 500) % 2) {
            display.drawBox(110, 22, 18, 12);
            display.setDrawColor(0);
            display.drawStr(112, 32, "SOS");
            display.setDrawColor(1);
        }
    } else {
        display.drawStr(0, 24, "Emergency broadcast");
        display.drawStr(0, 36, "will send help message");
        display.drawStr(0, 48, "with location data");
    }
    
    display.drawLine(0, 52, 128, 52);
    if (sosActive) {
        display.drawStr(2, 62, "B/D: STOP SOS");
    } else {
        display.drawStr(2, 62, "Any key: START  B: Back");
    }
    
    display.sendBuffer();
}

void startSOS() {
    sosActive = true;
    sosCounter = 0;
    lastSOSTransmission = 0; // Force immediate first transmission
    
    // SOS Emergency activated
    
    // Log SOS activation
    File logFile = SD.open("/log.txt", FILE_APPEND);
    if (logFile) {
        logFile.println("SOS: Emergency activated");
        logFile.close();
    }
}

void stopSOS() {
    sosActive = false;
    sosCounter = 0;
    
    // SOS Emergency deactivated
    
    // Log SOS deactivation
    File logFile = SD.open("/log.txt", FILE_APPEND);
    if (logFile) {
        logFile.println("SOS: Emergency deactivated");
        logFile.close();
    }
}

void sendSOSMessage() {
    if (!sosActive) return;
    
    char sosMessage[256];
    char messageId[32];
    generateMessageId(messageId, sizeof(messageId));
    
    // Create SOS message
    char* messageContent = "HELP!";
    if (gpsFixAvailable) {
        // Include GPS data if available
        snprintf(sosMessage, sizeof(sosMessage), "MSG:%s:%s|%s", messageId, messageContent, currentGPSData);
    } else if (hasLastKnownGPS) {
        // Use last known GPS data if available
        snprintf(sosMessage, sizeof(sosMessage), "MSG:%s:%s|%s", messageId, messageContent, lastKnownGPSData);
    } else {
        // No GPS data available
        snprintf(sosMessage, sizeof(sosMessage), "MSG:%s:%s", messageId, messageContent);
    }
    
    // Send SOS message
    LoRa.beginPacket();
    LoRa.print(sosMessage);
    LoRa.endPacket();
    
    // Log SOS message to SD card
    File logFile = SD.open("/log.txt", FILE_APPEND);
    if (logFile) {
        logFile.println("SENT: " + String(sosMessage));
        logFile.close();
    }
    
    // Increment SOS counter
    sosCounter++;
    
    // Update last transmission time
    lastSOSTransmission = millis();
}

// --- Main Program ---
void setup() {
    Serial.begin(115200);
    // Starting Offline Communication Mesh
    
    // Load frequency settings from EEPROM first
    loadFrequencySettings();
    
    // Initialize I2C and display
    Wire.begin(OLED_SDA, OLED_SCL);
    
    // Initialize display with error checking
    if (!display.begin()) {
        // Display initialization failed
    } else {
        // Display initialized successfully
    }
    
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB08_tr);
    display.setContrast(255);
    
    // Show startup message
    display.drawStr(0, 15, "Offline Comm Mesh");
    display.drawStr(0, 30, "Initializing...");
    display.sendBuffer();
    
    Serial2.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    
    // Initialize EEPROM
    EEPROM.begin(512);
    loadWifiCredentials();
    
    // Initialize hardware with retries
    int initAttempts = 0;
    while (!initLoRa() && initAttempts < 3) {
        // LoRa init failed, retrying
        initAttempts++;
        if (initAttempts >= 3) break;
        delay(1000);
    }
    
    initAttempts = 0;
    while (!initSDCard() && initAttempts < 3) {
        // SD card init failed, retrying
        initAttempts++;
        if (initAttempts >= 3) break;
        delay(1000);
    }
    
    // Load GPS data after SD card is initialized
    loadGPSFromSD();
    
    // Initialize WiFi in station mode
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    // Connect to WiFi if credentials are stored
    if (wifiConfigured) {
        display.clearBuffer();
        display.drawStr(0, 15, "Connecting to WiFi...");
        display.drawStr(0, 30, wifiSSID);
        display.sendBuffer();
        
        WiFi.begin(wifiSSID, wifiPassword);
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            display.clearBuffer();
            display.drawStr(0, 15, "WiFi Connected!");
            display.drawStr(0, 30, ("IP: " + WiFi.localIP().toString()).c_str());
            display.sendBuffer();
            
            // Start web server
            setupWebServer();
            delay(2000);
        } else {
            display.clearBuffer();
            display.drawStr(0, 15, "WiFi Failed!");
            display.drawStr(0, 30, "Use Settings > WiFi");
            display.drawStr(0, 40, "to reconnect");
            display.sendBuffer();
            delay(2000);
            // Don't set wifiConfigured to false - keep credentials for retry
            // wifiConfigured = false;
        }
    }
    
    randomSeed(analogRead(0));
    resetWatchdog();
    
    // Go directly to menu
    showMenu();
}

void loop() {
    // Process GPS data
    while (Serial2.available()) {
        gps.encode(Serial2.read());
    }
    
    // Update GPS data
    updateGPSData();

    // Handle web server requests if WiFi is connected
    if (wifiConfigured) {
        server.handleClient();
    }

    // Handle message sending and acknowledgment
    if (currentState == STATE_WAITING_ACK) {
        if (millis() - lastSendTime > ACK_TIMEOUT && lastSendTime != 0) {
            // Timeout waiting for ACK, retry
            sendMessageWithRetry();
        } else if (lastSendTime == 0) {
            // First time sending
            sendMessageWithRetry();
        }
        
        // Check for incoming acknowledgments
        checkForAck();
    } else if (currentState != STATE_VIEW_MESSAGES && currentState != STATE_MESSAGE_DETAIL && 
               currentState != STATE_MESSAGE_OPTIONS && currentState != STATE_DELETE_CONFIRM && 
               currentState != STATE_FORWARD_MESSAGE && currentState != STATE_VIEW_GPS_LOCATION &&
               currentState != STATE_WIFI_SCAN && currentState != STATE_WIFI_SELECT && 
               currentState != STATE_WIFI_PASSWORD) {
        // Handle received messages (when not viewing messages)
        if (newMessageReceived) {
            newMessageReceived = false;
            
            if (strncmp(receivedMessage, "MSG:", 4) == 0) {
                // Extract message ID and content
                char* firstColon = strchr(receivedMessage, ':');
                char* secondColon = firstColon ? strchr(firstColon + 1, ':') : NULL;
                
                if (firstColon && secondColon) {
                    // Extract message ID
                    size_t idLength = secondColon - (firstColon + 1);
                    if (idLength < sizeof(receivedId)) {
                        strncpy(receivedId, firstColon + 1, idLength);
                        receivedId[idLength] = '\0';
                    }
                    
                    // Extract message content (which may include GPS data)
                    char* messageContent = secondColon + 1;
                    char originalMessage[128] = "";
                    
                    // Check if message contains GPS data (separated by |)
                    char* gpsSeparator = strchr(messageContent, '|');
                    if (gpsSeparator) {
                        // Split into message and GPS data
                        size_t msgLength = gpsSeparator - messageContent;
                        strncpy(originalMessage, messageContent, msgLength);
                        originalMessage[msgLength] = '\0';
                        strncpy(receivedGPSData, gpsSeparator + 1, sizeof(receivedGPSData) - 1);
                        receivedGPSData[sizeof(receivedGPSData) - 1] = '\0';
                    } else {
                        // No GPS data, use the whole content as message
                        strncpy(originalMessage, messageContent, sizeof(originalMessage) - 1);
                        originalMessage[sizeof(originalMessage) - 1] = '\0';
                    }
                    
                    // Display received message
                    display.clearBuffer();
                    display.drawStr(0, 15, "RX Msg:");
                    
                    // Display message content
                    if (strlen(originalMessage) > 21) {
                        char line1[22];
                        char line2[22];
                        strncpy(line1, originalMessage, 21);
                        line1[21] = '\0';
                        strncpy(line2, originalMessage + 21, 21);
                        line2[21] = '\0';
                        display.drawStr(0, 30, line1);
                        display.drawStr(0, 45, line2);
                    } else {
                        display.drawStr(0, 30, originalMessage);
                    }
                    
                    // Display GPS indicator if available
                    if (strlen(receivedGPSData) > 0) {
                        display.drawStr(0, 55, "GPS data included");
                    }
                    
                    display.drawStr(0, 60, "B=Back");
                    display.sendBuffer();

                    // Log to SD card (include GPS info in log)
                    File logFile = SD.open("/log.txt", FILE_APPEND);
                    if (logFile) {
                        if (strlen(receivedGPSData) > 0) {
                            logFile.println("RCVD: " + String(receivedMessage) + " [GPS]");
                        } else {
                            logFile.println("RCVD: " + String(receivedMessage));
                        }
                        logFile.close();
                    }
                    
                    // Send acknowledgment
                    char ackMessage[64];
                    snprintf(ackMessage, sizeof(ackMessage), "ACK:%s", receivedId);
                    LoRa.beginPacket();
                    LoRa.print(ackMessage);
                    LoRa.endPacket();
                    
                    // Log ACK to SD card
                    logFile = SD.open("/log.txt", FILE_APPEND);
                    if (logFile) {
                        logFile.println("SENT: " + String(ackMessage));
                        logFile.close();
                    }
                    
                    currentState = STATE_LORA_RX;
                }
            }
        }
    }

    // Handle SOS transmission timing
    if (sosActive && (millis() - lastSOSTransmission >= SOS_INTERVAL)) {
        sendSOSMessage();
        if (currentState == STATE_SOS_EMERGENCY) {
            executeState(); // Update display with new count
        }
    }

    // Handle keypad input
    char key = keypad.getKey();
    if (key) {
        if (currentState == STATE_WIFI_SCAN || currentState == STATE_WIFI_SELECT || currentState == STATE_WIFI_PASSWORD) {
            // Handle WiFi-related states separately
            handleKeypadInput(key);
        } else {
            handleKeypadInput(key);
        }
        delay(100); // Small delay to prevent too rapid input
    }
    
    // Auto-confirm letter after idle period for text input
    if (currentState == STATE_SEND_MESSAGE && lastKey != -1 && (millis() - lastPress > LETTER_TIMEOUT)) {
        msgIndex++;
        lastKey = -1;
        executeState();
    }
    
    // Auto-confirm letter after idle period for WiFi password input
    if (currentState == STATE_WIFI_PASSWORD && wifiLastKey != -1 && (millis() - wifiLastPress > LETTER_TIMEOUT)) {
        wifiInputIndex++;
        wifiLastKey = -1;
        executeState();
    }
    
    // Periodically update display for GPS info
    static unsigned long lastUpdate = 0;
    if (currentState == STATE_GPS_INFO && millis() - lastUpdate > 1000) {
        executeState();
        lastUpdate = millis();
    }
    
    // Make sure we're always in receive mode
    if (loraInitialized) {
        LoRa.receive();
    }
    
    // Check watchdog timer
    checkWatchdog();
    
    // Small delay to prevent busy waiting
    delay(10);
}

// --- Frequency Management Functions ---
void loadFrequencySettings() {
    EEPROM.begin(512);
    int savedFreq = EEPROM.read(EEPROM_FREQ_ADDR);
    if (savedFreq == 0 || savedFreq == 1) {
        loraFrequency = savedFreq;
            // Loaded frequency setting
    } else {
        loraFrequency = 0; // Default to 433MHz
        // Using default frequency: 433 MHz
    }
    EEPROM.end();
}

void saveFrequencySettings() {
    EEPROM.begin(512);
    EEPROM.write(EEPROM_FREQ_ADDR, loraFrequency);
    EEPROM.commit();
    EEPROM.end();
    // Saved frequency setting
}

void setLoRaFrequency() {
    // Simple frequency change without blocking operations
    // LoRa frequency will change
    
    // Mark that LoRa needs reinitialization on next use
    loraInitialized = false;
}

void showFrequencySelectScreen() {
    display.clearBuffer();
    display.setFont(u8g2_font_6x10_tr);
    
    display.drawStr(0, 10, "LoRa Frequency");
    display.drawLine(0, 12, 128, 12);
    
    // Show current frequency
    display.drawStr(0, 22, "Current: 433 MHz");
    if (loraFrequency == 1) {
        display.drawStr(0, 22, "Current: 169 MHz");
    }
    
    // Show frequency options
    display.drawStr(8, 35, "433 MHz");
    display.drawStr(8, 45, "169 MHz");
    
    // Show selection indicator
    if (frequencyIndex == 0) {
        display.drawStr(0, 35, ">");
    } else if (frequencyIndex == 1) {
        display.drawStr(0, 45, ">");
    }
    
    display.drawStr(0, 60, "2/8=Nav 5=Select B=Back");
    
    display.sendBuffer();
}

void handleFrequencySelectInput(char key) {
    switch(key) {
        case '2':
            if (frequencyIndex > 0) {
                frequencyIndex--;
            }
            break;
            
        case '8':
            if (frequencyIndex < FREQUENCY_COUNT - 1) {
                frequencyIndex++;
            }
            break;
            
        case '5':
            if (loraFrequency != frequencyIndex) {
                loraFrequency = frequencyIndex;
                saveFrequencySettings();
                setLoRaFrequency();
            }
            currentState = STATE_SETTINGS;
            break;
            
        case 'B':
        case 'b':
        case 'D':
        case 'd':
            currentState = STATE_SETTINGS;
            break;
    }
}

// Save GPS data to SD card
void saveGPSToSD() {
    File gpsFile = SD.open("/gps.txt", FILE_WRITE);
    if (gpsFile) {
        gpsFile.println(lastKnownGPSData);
        gpsFile.close();
        // Saved GPS data
    } else {
        // Failed to save GPS data to SD card
    }
}

// Load GPS data from SD card
void loadGPSFromSD() {
    File gpsFile = SD.open("/gps.txt", FILE_READ);
    if (gpsFile) {
        if (gpsFile.available()) {
            String gpsData = gpsFile.readStringUntil('\n');
            gpsData.trim();
            if (gpsData.length() > 0 && gpsData.length() < sizeof(lastKnownGPSData)) {
                strcpy(lastKnownGPSData, gpsData.c_str());
                hasLastKnownGPS = true;
                // Loaded GPS data
            }
        }
        gpsFile.close();
    } else {
        // No GPS data file found
    }
}