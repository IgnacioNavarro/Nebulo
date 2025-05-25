// src/main.cpp
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>

// WiFi credentials
const char* ssid = "Apple Network NB";
const char* password = "AirportNB0009,";

// API endpoints
const char* bitaxe_url = "http://192.168.1.140";

// Display setup
TFT_eSPI tft = TFT_eSPI();

// Timing variables
unsigned long lastApiCall = 0;

const unsigned long apiInterval = 10000; // 10 seconds in milliseconds

// Function declarations (required in C++)
void connectToWiFi();
void fetchAndDisplayData();
String callAPI(const char* url);
void displayBitAxeData(String jsonData);
void handleAPIError(String error);

void setup() {
  Serial.begin(115200);
  
  // Initialize display
  tft.init();
  tft.setRotation(1); // Landscape mode
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  
  // Connect to WiFi
  connectToWiFi();
  
  // Initial API calls
  fetchAndDisplayData();
}

void loop() {
  // Check if it's time for API calls
  if (millis() - lastApiCall >= apiInterval) {
    fetchAndDisplayData();
    lastApiCall = millis();
  }
  
  // Handle WiFi reconnection if needed
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }
  
  delay(1000); // Small delay to prevent excessive polling
}

void connectToWiFi() {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.println("Connecting to WiFi...");
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    tft.print(".");
  }
  
  Serial.println("\nWiFi connected!");
  tft.println("\nConnected!");
  delay(1000);
}

void fetchAndDisplayData() {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.setTextSize(2);
  
  // Display header
  tft.setTextColor(TFT_YELLOW);
  tft.println("BitAxe Monitor");
  tft.setTextSize(1);
  tft.println("Last Update: " + String(millis()/1000) + "s");
  tft.println("");
  
  // Call BitAxe API
  String fullUrl = String(bitaxe_url) + "/api/system/info";
  String bitaxeData = callAPI(fullUrl.c_str());
  displayBitAxeData(bitaxeData);
}

String callAPI(const char* url) {
  HTTPClient http;
  http.begin(url);
  
  // Add headers if needed (e.g., API keys)
  // http.addHeader("Authorization", "Bearer YOUR_API_KEY");
  
  int httpCode = http.GET();
  String payload = "";
  
  if (httpCode == HTTP_CODE_OK) {
    payload = http.getString();
    Serial.println("API Response: " + payload);
  } else {
    Serial.println("HTTP Error: " + String(httpCode));
    payload = "Error: " + String(httpCode);
  }
  
  http.end();
  return payload;
}

void displayBitAxeData(String jsonData) {
  // Parse JSON
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonData);
  
  if (error) {
    tft.setTextColor(TFT_RED);
    tft.setTextSize(1);
    tft.println("JSON Parse Error:");
    tft.println(error.c_str());
    Serial.println("JSON Parse Error: " + String(error.c_str()));
    return;
  }
  
  // Display BitAxe data with nice formatting
  tft.setTextSize(1);
  
  // Temperature (critical info - make it prominent)
  if (!doc["temp"].isNull()) {
    tft.setTextColor(TFT_RED);
    tft.setTextSize(2);
    tft.println("Temp: " + String(doc["temp"].as<int>()) + "C");
    tft.setTextSize(1);
    tft.println("");
  }
  
  // Hash Rate
  if (!doc["hashRate"].isNull()) {
    tft.setTextColor(TFT_GREEN);
    float hashRate = doc["hashRate"].as<float>();
    tft.println("Hash Rate: " + String(hashRate, 1) + " GH/s");
    tft.println("");
  }
  
  // Best Difficulty
  if (!doc["bestDiff"].isNull()) {
    tft.setTextColor(TFT_CYAN);
    tft.println("Best Diff: " + String(doc["bestDiff"].as<String>()));
  }
  
  // Best Session Difficulty  
  if (!doc["bestSessionDiff"].isNull()) {
    tft.setTextColor(TFT_CYAN);
    tft.println("Session Diff: " + String(doc["bestSessionDiff"].as<String>()));
    tft.println("");
  }
  
  // Frequency
  if (!doc["frequency"].isNull()) {
    tft.setTextColor(TFT_MAGENTA);
    tft.println("Frequency: " + String(doc["frequency"].as<int>()) + " MHz");
  }
  
  // Core Voltage
  if (!doc["coreVoltage"].isNull()) {
    tft.setTextColor(TFT_MAGENTA);
    int voltage = doc["coreVoltage"].as<int>();
    tft.println("Core Voltage: " + String(voltage) + " mV");
  }
  
  // Additional useful info
  tft.println("");
  tft.setTextColor(TFT_WHITE);
  
  // Power consumption
  if (!doc["power"].isNull()) {
    float power = doc["power"].as<float>();
    tft.println("Power: " + String(power, 1) + "W");
  }
  
  // Uptime
  if (!doc["uptimeSeconds"].isNull()) {
    unsigned long uptime = doc["uptimeSeconds"].as<unsigned long>();
    unsigned long days = uptime / 86400;
    unsigned long hours = (uptime % 86400) / 3600;
    tft.println("Uptime: " + String(days) + "d " + String(hours) + "h");
  }
}

// Optional: Add error handling and retry logic
void handleAPIError(String error) {
  tft.setTextColor(TFT_RED);
  tft.println("API Error:");
  tft.println(error);
  Serial.println("API Error: " + error);
}