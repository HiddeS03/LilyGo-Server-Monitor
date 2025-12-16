/**
 * WiFi Monitor for LilyGo T5-ePaper-S3
 *
 * Connects to WiFi and displays connection status on e-paper display.
 * This is the first step in building a Docker server monitoring system.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "epd_driver.h"
#include "firasans.h"
#include "utilities.h"
#include "credentials.h"

// ============================================================================
// Configuration
// ============================================================================

// WiFi credentials are now in credentials.h (gitignored)
// Copy credentials.h.example to credentials.h and fill in your details

// Update interval in milliseconds (60 seconds)
const unsigned long UPDATE_INTERVAL = 60000;

// ============================================================================
// Global Variables
// ============================================================================

uint8_t *framebuffer = NULL;
unsigned long lastUpdate = 0;

// ============================================================================
// Display Functions
// ============================================================================

/**
 * Clear the framebuffer (set to white)
 */
void clearFramebuffer()
{
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
}

/**
 * Display text on the e-paper at specified position
 */
void displayText(const char *text, int x, int y)
{
  int cursor_x = x;
  int cursor_y = y;
  writeln(&FiraSans, text, &cursor_x, &cursor_y, framebuffer);
}

/**
 * Display Docker metrics on e-paper
 */
void displayDockerMetrics(const char *status, int totalContainers, int runningContainers,
                          float cpuPercent, float memoryPercent)
{
  epd_poweron();
  epd_clear();
  clearFramebuffer();

  int cursor_x = 50;
  int cursor_y = 80;

  // Title
  writeln(&FiraSans, "Docker Server Monitor", &cursor_x, &cursor_y, framebuffer);
  cursor_y += FiraSans.advance_y + 10;

  cursor_x = 50;
  char statusMsg[100];
  snprintf(statusMsg, sizeof(statusMsg), "Status: %s", status);
  writeln(&FiraSans, statusMsg, &cursor_x, &cursor_y, framebuffer);

  // Horizontal line
  cursor_y += 20;
  epd_draw_hline(50, cursor_y, EPD_WIDTH - 100, 0, framebuffer);
  cursor_y += 30;

  // Container stats
  cursor_x = 50;
  writeln(&FiraSans, "CONTAINERS:", &cursor_x, &cursor_y, framebuffer);
  cursor_y += FiraSans.advance_y + 5;

  cursor_x = 70;
  char containerMsg[100];
  snprintf(containerMsg, sizeof(containerMsg), "Running: %d / %d", runningContainers, totalContainers);
  writeln(&FiraSans, containerMsg, &cursor_x, &cursor_y, framebuffer);
  cursor_y += FiraSans.advance_y + 5;

  cursor_x = 70;
  snprintf(containerMsg, sizeof(containerMsg), "Stopped: %d", totalContainers - runningContainers);
  writeln(&FiraSans, containerMsg, &cursor_x, &cursor_y, framebuffer);

  cursor_y += FiraSans.advance_y + 20;

  // Resource usage
  cursor_x = 50;
  writeln(&FiraSans, "RESOURCES:", &cursor_x, &cursor_y, framebuffer);
  cursor_y += FiraSans.advance_y + 5;

  cursor_x = 70;
  char resourceMsg[100];
  snprintf(resourceMsg, sizeof(resourceMsg), "CPU:    %.1f%%", cpuPercent);
  writeln(&FiraSans, resourceMsg, &cursor_x, &cursor_y, framebuffer);
  cursor_y += FiraSans.advance_y + 5;

  cursor_x = 70;
  snprintf(resourceMsg, sizeof(resourceMsg), "Memory: %.1f%%", memoryPercent);
  writeln(&FiraSans, resourceMsg, &cursor_x, &cursor_y, framebuffer);

  // Update timestamp
  cursor_y += FiraSans.advance_y + 30;
  cursor_x = 50;
  char timeMsg[100];
  snprintf(timeMsg, sizeof(timeMsg), "Updated: %lu sec ago", millis() / 1000);
  writeln(&FiraSans, timeMsg, &cursor_x, &cursor_y, framebuffer);

  epd_draw_grayscale_image(epd_full_screen(), framebuffer);
  epd_poweroff();
}

/**
 * Display connection status on e-paper
 */
void displayConnectionStatus(bool connected, const char *ipAddress = "")
{
  epd_poweron();
  epd_clear();
  clearFramebuffer();

  int cursor_x = 50;
  int cursor_y = 100;

  // Title
  writeln(&FiraSans, "Docker Server Monitor", &cursor_x, &cursor_y, framebuffer);
  cursor_y += FiraSans.advance_y + 20;

  // WiFi Status
  cursor_x = 50;
  if (connected)
  {
    writeln(&FiraSans, "WiFi: Connected", &cursor_x, &cursor_y, framebuffer);
    cursor_y += FiraSans.advance_y + 10;

    cursor_x = 50;
    char ipMsg[100];
    snprintf(ipMsg, sizeof(ipMsg), "IP: %s", ipAddress);
    writeln(&FiraSans, ipMsg, &cursor_x, &cursor_y, framebuffer);

    cursor_y += FiraSans.advance_y + 10;
    cursor_x = 50;
    char rssiMsg[50];
    snprintf(rssiMsg, sizeof(rssiMsg), "Signal: %d dBm", WiFi.RSSI());
    writeln(&FiraSans, rssiMsg, &cursor_x, &cursor_y, framebuffer);
  }
  else
  {
    writeln(&FiraSans, "WiFi: Not Connected", &cursor_x, &cursor_y, framebuffer);
    cursor_y += FiraSans.advance_y + 10;
    cursor_x = 50;
    writeln(&FiraSans, "Attempting to connect...", &cursor_x, &cursor_y, framebuffer);
  }

  // Draw horizontal line
  cursor_y += 30;
  epd_draw_hline(50, cursor_y, EPD_WIDTH - 100, 0, framebuffer);

  cursor_y += 40;
  cursor_x = 50;

  // Status message
  writeln(&FiraSans, "Status: Monitoring initialized", &cursor_x, &cursor_y, framebuffer);

  // Update display
  epd_draw_grayscale_image(epd_full_screen(), framebuffer);
  epd_poweroff();
}

/**
 * Connect to WiFi with timeout
 */
bool connectToWiFi()
{
  Serial.println("Connecting to WiFi...");
  Serial.print("SSID: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Display connecting status
  displayConnectionStatus(false);

  int attempts = 0;
  const int maxAttempts = 20; // 10 seconds timeout

  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts)
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("WiFi connected successfully!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    return true;
  }
  else
  {
    Serial.println("WiFi connection failed!");
    return false;
  }
}

// ============================================================================
// Setup & Loop
// ============================================================================

void setup()
{
  // Initialize serial
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=================================");
  Serial.println("WiFi Monitor for Docker Servers");
  Serial.println("=================================\n");

  // Allocate framebuffer in PSRAM
  Serial.println("Allocating framebuffer...");
  framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
  if (!framebuffer)
  {
    Serial.println("ERROR: Framebuffer allocation failed!");
    while (1)
    {
      delay(1000);
    }
  }
  Serial.println("Framebuffer allocated successfully");

  // Initialize e-paper display
  Serial.println("Initializing e-paper display...");
  epd_init();
  epd_poweron();
  epd_clear();
  epd_poweroff();
  Serial.println("Display initialized");

  // Connect to WiFi
  bool wifiConnected = connectToWiFi();

  if (wifiConnected)
  {
    // Display connected status
    displayConnectionStatus(true, WiFi.localIP().toString().c_str());
  }
  else
  {
    // Display failed status
    displayConnectionStatus(false);
  }

  Serial.println("\nSetup complete!");
  Serial.println("Entering main loop...\n");
}

void loop()
{
  // Check if we need to update
  unsigned long currentTime = millis();

  // Handle WiFi connection
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi connection lost. Reconnecting...");
    displayConnectionStatus(false);

    if (connectToWiFi())
    {
      displayConnectionStatus(true, WiFi.localIP().toString().c_str());
    }
  }

  // Periodic status update
  if (currentTime - lastUpdate >= UPDATE_INTERVAL)
  {
    lastUpdate = currentTime;

    Serial.println("\n--- Status Update ---");
    Serial.print("WiFi Status: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");

    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      Serial.print("Signal Strength: ");
      Serial.print(WiFi.RSSI());
      Serial.println(" dBm");

      // Fetch data from server
      Serial.println("Fetching data from server...");
      HTTPClient http;
      http.setTimeout(10000); // 10 second timeout
      http.begin(SERVER_URL);

      int httpCode = http.GET();
      Serial.print("HTTP Response Code: ");
      Serial.println(httpCode);

      if (httpCode == HTTP_CODE_OK)
      {
        String payload = http.getString();
        Serial.println("Received payload:");
        Serial.println(payload);

        // Parse JSON
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error)
        {
          const char *status = doc["status"];
          int totalContainers = doc["containers"]["total"];
          int runningContainers = doc["containers"]["running"];
          float cpuPercent = doc["resources"]["cpu_percent"];
          float memoryPercent = doc["resources"]["memory_percent"];

          Serial.println("\nParsed data:");
          Serial.printf("  Status: %s\n", status);
          Serial.printf("  Containers: %d/%d running\n", runningContainers, totalContainers);
          Serial.printf("  CPU: %.1f%%\n", cpuPercent);
          Serial.printf("  Memory: %.1f%%\n", memoryPercent);

          // Display metrics on e-paper
          displayDockerMetrics(status, totalContainers, runningContainers,
                               cpuPercent, memoryPercent);
        }
        else
        {
          Serial.print("JSON parsing failed: ");
          Serial.println(error.c_str());
          displayConnectionStatus(true, "JSON Parse Error");
        }
      }
      else
      {
        Serial.print("HTTP request failed: ");
        Serial.println(httpCode);
        displayConnectionStatus(true, "Server Error");
      }

      http.end();
    }

    Serial.print("Free heap: ");
    Serial.println(ESP.getFreeHeap());
    Serial.print("Free PSRAM: ");
    Serial.println(ESP.getFreePsram());
  }

  delay(1000); // Check every second
}
