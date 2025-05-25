// src/main.cpp
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <time.h>

// Configuration constants
namespace Config {
    // Display
    constexpr uint8_t SCREEN_ROTATION = 1;  // 0=Portrait, 1=Landscape, 2=Inv Portrait, 3=Inv Landscape
    constexpr uint16_t REFRESH_INTERVAL = 10000;  // API refresh interval in ms
    
    // Screen positions
    namespace Display {
        constexpr uint8_t MARGIN_LEFT = 10;
        constexpr uint8_t LINE_HEIGHT = 15;
        constexpr uint8_t HEADER_Y = 10;
        constexpr uint8_t TIME_Y = 30;
        constexpr uint8_t TEMP_Y = 45;
        constexpr uint8_t HASHRATE_Y = 60;
        constexpr uint8_t DIFF_Y = 75;
        constexpr uint8_t SESSION_DIFF_Y = 90;
        constexpr uint8_t FREQ_Y = 105;
        constexpr uint8_t VOLTAGE_Y = 120;
        constexpr uint8_t POWER_Y = 135;
    }
    
    // Network
    const char* WIFI_SSID = "Apple Network NB";
    const char* WIFI_PASSWORD = "AirportNB0009,";
    const char* BITAXE_URL = "http://192.168.1.140";
    const char* NTP_SERVER = "pool.ntp.org";
    constexpr long GMT_OFFSET_SEC = 3600;     // GMT+1
    constexpr int DAYLIGHT_OFFSET_SEC = 3600; // Summer time
    
    // Temperature thresholds
    constexpr int TEMP_WARNING = 70;  // Temperature warning threshold in Celsius
}

// Global objects
TFT_eSPI tft = TFT_eSPI();
unsigned long lastApiCall = 0;

// Forward declarations
void setupDisplay();
void setupWiFi();
void setupTime();
void clearLine(int y, int height = 20);
void displayValue(const char* label, int y, const char* value, uint16_t color);
String getFormattedTime();
String fetchBitaxeData();
void displayBitaxeData(const String& jsonData);
void handleError(const char* message);
void fetchAndDisplayData();

// Helper functions implementation
void clearLine(int y, int height) {
    tft.fillRect(0, y, tft.width(), height, TFT_BLACK);
}

void displayValue(const char* label, int y, const char* value, uint16_t color) {
    clearLine(y);
    tft.setCursor(Config::Display::MARGIN_LEFT, y);
    tft.setTextColor(color);
    tft.print(label);
    tft.print(value);
}

String getFormattedTime() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) {
        return "Time Error";
    }
    char timeString[9];
    snprintf(timeString, sizeof(timeString), "%02d:%02d:%02d", 
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return String(timeString);
}

void handleError(const char* message) {
    Serial.println(message);
    tft.setTextColor(TFT_RED);
    tft.println(message);
}

// Setup functions
void setupDisplay() {
    tft.init();
    tft.setRotation(Config::SCREEN_ROTATION);
    tft.fillScreen(TFT_BLACK);
    tft.invertDisplay(true);
    
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    
    // Draw header
    tft.setTextSize(2);
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(Config::Display::MARGIN_LEFT, Config::Display::HEADER_Y);
    tft.println("BitAxe Monitor");
    tft.setTextSize(1);
}

void setupWiFi() {
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(Config::Display::MARGIN_LEFT, Config::Display::TIME_Y);
    tft.print("Connecting to WiFi...");
    
    WiFi.begin(Config::WIFI_SSID, Config::WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
        tft.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        clearLine(Config::Display::TIME_Y);
        tft.setCursor(Config::Display::MARGIN_LEFT, Config::Display::TIME_Y);
        tft.setTextColor(TFT_GREEN);
        tft.println("WiFi Connected!");
    } else {
        handleError("WiFi connection failed");
    }
}

void setupTime() {
    configTime(Config::GMT_OFFSET_SEC, Config::DAYLIGHT_OFFSET_SEC, Config::NTP_SERVER);
}

// Data handling functions
String fetchBitaxeData() {
    String fullUrl = String(Config::BITAXE_URL) + "/api/system/info";
    HTTPClient http;
    http.begin(fullUrl);
    
    int httpCode = http.GET();
    String payload = "";
    
    if (httpCode == HTTP_CODE_OK) {
        payload = http.getString();
    } else {
        handleError("API Error");
    }
    
    http.end();
    return payload;
}

void fetchAndDisplayData() {
    String jsonData = fetchBitaxeData();
    if (!jsonData.isEmpty()) {
        displayBitaxeData(jsonData);
    }
}

void displayBitaxeData(const String& jsonData) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonData);
    
    if (error) {
        handleError("JSON Parse Error");
        return;
    }
    
    // Update timestamp
    clearLine(Config::Display::TIME_Y);
    tft.setCursor(Config::Display::MARGIN_LEFT, Config::Display::TIME_Y);
    tft.setTextColor(TFT_WHITE);
    tft.print("Last Update: ");
    tft.print(getFormattedTime());
    
    char buffer[32];
    
    // Display all values
    if (!doc["temp"].isNull()) {
        int temp = doc["temp"].as<int>();
        uint16_t tempColor = (temp > Config::TEMP_WARNING) ? TFT_RED : TFT_WHITE;
        snprintf(buffer, sizeof(buffer), "%dC", temp);
        displayValue("Temp: ", Config::Display::TEMP_Y, buffer, tempColor);
    }
    
    if (!doc["hashRate"].isNull()) {
        float hashRate = doc["hashRate"].as<float>();
        snprintf(buffer, sizeof(buffer), "%.1f GH/s", hashRate);
        displayValue("Hash Rate: ", Config::Display::HASHRATE_Y, buffer, TFT_GREEN);
    }
    
    if (!doc["bestDiff"].isNull()) {
        const char* bestDiff = doc["bestDiff"].as<const char*>();
        displayValue("Best Diff: ", Config::Display::DIFF_Y, bestDiff, TFT_CYAN);
    }
    
    if (!doc["bestSessionDiff"].isNull()) {
        const char* sessionDiff = doc["bestSessionDiff"].as<const char*>();
        displayValue("Session Diff: ", Config::Display::SESSION_DIFF_Y, sessionDiff, TFT_CYAN);
    }
    
    if (!doc["frequency"].isNull()) {
        int freq = doc["frequency"].as<int>();
        snprintf(buffer, sizeof(buffer), "%d MHz", freq);
        displayValue("Frequency: ", Config::Display::FREQ_Y, buffer, TFT_MAGENTA);
    }
    
    if (!doc["coreVoltage"].isNull()) {
        int voltage = doc["coreVoltage"].as<int>();
        snprintf(buffer, sizeof(buffer), "%d mV", voltage);
        displayValue("Core Voltage: ", Config::Display::VOLTAGE_Y, buffer, TFT_MAGENTA);
    }
    
    if (!doc["power"].isNull()) {
        float power = doc["power"].as<float>();
        snprintf(buffer, sizeof(buffer), "%.1fW", power);
        displayValue("Power: ", Config::Display::POWER_Y, buffer, TFT_WHITE);
    }
}

// Main program functions
void setup() {
    Serial.begin(115200);
    setupDisplay();
    setupWiFi();
    setupTime();
    fetchAndDisplayData();
}

void loop() {
    if (millis() - lastApiCall >= Config::REFRESH_INTERVAL) {
        fetchAndDisplayData();
        lastApiCall = millis();
    }
    
    // Handle WiFi reconnection if needed
    if (WiFi.status() != WL_CONNECTED) {
        setupWiFi();
        fetchAndDisplayData();
    }
    
    delay(1000);
}
