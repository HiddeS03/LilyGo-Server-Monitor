/**
 * Docker Game Server Monitor for LilyGo T5-ePaper-S3
 *
 * Monitors Minecraft Bingo, Minecraft, and Satisfactory servers
 * Shows: online status, player counts, logs, CPU temp, RAM usage
 * Updates every 5 seconds with partial screen refresh
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "epd_driver.h"
#include "font/firasans_small.h"
#include "utilities.h"
#include "credentials.h"

// ============================================================================
// Configuration
// ============================================================================

// Update interval: 5 seconds
const unsigned long UPDATE_INTERVAL = 5000;

// ============================================================================
// Global Variables
// ============================================================================

uint8_t *framebuffer = NULL;
unsigned long lastUpdate = 0;
bool firstUpdate = true;

// Previous values to detect changes
struct ServerState
{
  bool online;
  int players;
  String log1;
  String log2;
  String log3;
};

ServerState prevBingo = {false, 0, "", "", ""};
ServerState prevMinecraft = {false, 0, "", "", ""};
ServerState prevSatisfactory = {false, 0, "", "", ""};
float prevCpuTemp = 0.0;
float prevMemory = 0.0;

// ============================================================================
// Display Helper Functions
// ============================================================================

/**
 * Clear a specific rectangular area on the display
 */
void clearArea(int x, int y, int width, int height)
{
  Rect_t area = {
      .x = x,
      .y = y,
      .width = width,
      .height = height};
  epd_fill_rect(x, y, width, height, 255, framebuffer);
}

/**
 * Write text and return cursor position
 */
int writeText(const char *text, int x, int y)
{
  int cursor_x = x;
  int cursor_y = y;
  writeln(&FiraSans, text, &cursor_x, &cursor_y, framebuffer);
  return cursor_y;
}

/**
 * Write text with wrapping within a maximum width
 * Returns the final Y position after all wrapped lines
 */
int writeTextWrapped(const char *text, int x, int y, int maxWidth)
{
  String str = String(text);
  int curr_y = y;
  int start = 0;

  while (start < str.length())
  {
    // Find how many chars fit in maxWidth
    int end = start;
    int test_len = 1;
    int32_t text_w, text_h;

    // Binary search for max fitting chars (approximate)
    int max_chars = min((int)str.length() - start, maxWidth / 8); // rough estimate: 8px per char

    for (int i = 1; i <= max_chars && (start + i) <= str.length(); i++)
    {
      String substr = str.substring(start, start + i);
      int32_t x1, y1;
      get_text_bounds(&FiraSans, substr.c_str(), &x, &y, &x1, &y1, &text_w, &text_h, NULL);

      if (text_w <= maxWidth)
      {
        end = start + i;
      }
      else
      {
        break;
      }
    }

    if (end == start)
      end = start + 1; // At least 1 char

    // Write this line
    String line = str.substring(start, end);
    writeText(line.c_str(), x, curr_y);
    curr_y += (FiraSans.advance_y / 2) + 1;

    start = end;
  }

  return curr_y;
}

/**
 * Full display update - draw framebuffer to screen
 */
void updateDisplay()
{
  epd_poweron();
  epd_draw_grayscale_image(epd_full_screen(), framebuffer);
  epd_poweroff();
}

/**
 * Clear entire screen and framebuffer
 */
void clearDisplay()
{
  epd_poweron();
  epd_clear();
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
  epd_poweroff();
}

// ============================================================================
// Display Layout Functions
// ============================================================================

/**
 * Draw the static header
 */
void drawHeader()
{
  int y = 20;
  writeText("DOCKER MONITOR", 30, y);

  // Draw horizontal line under title
  y += (FiraSans.advance_y / 2) + 5;
  epd_draw_hline(20, y, EPD_WIDTH - 40, 0, framebuffer);
}

/**
 * Draw system stats (CPU temp, memory)
 */
void drawSystemStats(float cpuTemp, float memUsage)
{
  int x = 700;
  int y = 20;

  // Draw CPU with label and padding
  char tempStr[40];
  snprintf(tempStr, sizeof(tempStr), "CPU:%.1fC", cpuTemp);
  writeText(tempStr, x, y);

  // Draw RAM with label and padding
  char memStr[40];
  snprintf(memStr, sizeof(memStr), " RAM:%.0f%%", memUsage);
  writeText(memStr, x + 100, y);
}

/**
 * Draw a server status block with box
 */
void drawServerBlock(const char *name, ServerState &current,
                     int x, int y, int width, int height, bool hasPlayers)
{
  // Draw border box
  epd_draw_rect(x, y, width, height, 0, framebuffer);

  int padding = 5;
  int curr_y = y + padding;
  int text_x = x + padding;

  // Server name on left
  writeText(name, text_x, curr_y);

  // Status on right side
  int status_x = x + width - 40;
  if (current.online)
  {
    writeText("ON", status_x, curr_y);
  }
  else
  {
    writeText("OFF", status_x, curr_y);
  }

  curr_y += (FiraSans.advance_y / 2) + 5;

  // Player count (if applicable)
  if (hasPlayers && current.online)
  {
    char playerStr[30];
    snprintf(playerStr, sizeof(playerStr), "P:%d", current.players);
    writeText(playerStr, text_x, curr_y);
    curr_y += (FiraSans.advance_y / 2) + 4;
  }

  // Logs (only if online) - with text wrapping
  if (current.online)
  {
    int logWidth = width - (2 * padding);

    if (current.log1.length() > 0)
    {
      curr_y = writeTextWrapped(current.log1.c_str(), text_x, curr_y, logWidth);
      curr_y += 2;
    }
    if (current.log2.length() > 0)
    {
      curr_y = writeTextWrapped(current.log2.c_str(), text_x, curr_y, logWidth);
      curr_y += 2;
    }
    if (current.log3.length() > 0)
    {
      curr_y = writeTextWrapped(current.log3.c_str(), text_x, curr_y, logWidth);
    }
  }
}

// ============================================================================
// Network Functions
// ============================================================================

/**
 * Connect to WiFi
 */
bool connectToWiFi()
{
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20)
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("WiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println("WiFi connection failed!");
  return false;
}

/**
 * Fetch and display server data
 */
void fetchAndDisplayData(bool forceFullRedraw)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi not connected!");
    return;
  }

  Serial.println("Fetching server data...");
  HTTPClient http;
  http.setTimeout(5000);
  http.begin(SERVER_URL);

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK)
  {
    String payload = http.getString();

    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error)
    {
      // Extract system stats
      float cpuTemp = doc["system"]["cpu_temp"] | 0.0;
      float memUsage = doc["system"]["memory_percent"] | 0.0;

      // Extract server states
      ServerState bingo, minecraft, satisfactory;

      // Minecraft Bingo
      JsonObject bingoObj = doc["servers"]["minecraft_bingo"];
      bingo.online = bingoObj["online"] | false;
      bingo.players = bingoObj["players"] | 0;
      JsonArray bingoLogs = bingoObj["logs"];
      if (bingoLogs.size() > 0)
        bingo.log1 = bingoLogs[0].as<String>();
      if (bingoLogs.size() > 1)
        bingo.log2 = bingoLogs[1].as<String>();
      if (bingoLogs.size() > 2)
        bingo.log3 = bingoLogs[2].as<String>();

      // Minecraft
      JsonObject mcObj = doc["servers"]["minecraft"];
      minecraft.online = mcObj["online"] | false;
      minecraft.players = mcObj["players"] | 0;
      JsonArray mcLogs = mcObj["logs"];
      if (mcLogs.size() > 0)
        minecraft.log1 = mcLogs[0].as<String>();
      if (mcLogs.size() > 1)
        minecraft.log2 = mcLogs[1].as<String>();
      if (mcLogs.size() > 2)
        minecraft.log3 = mcLogs[2].as<String>();

      // Satisfactory
      JsonObject satObj = doc["servers"]["satisfactory"];
      satisfactory.online = satObj["online"] | false;
      satisfactory.players = 0; // Satisfactory doesn't have player count in this version
      JsonArray satLogs = satObj["logs"];
      if (satLogs.size() > 0)
        satisfactory.log1 = satLogs[0].as<String>();
      if (satLogs.size() > 1)
        satisfactory.log2 = satLogs[1].as<String>();
      if (satLogs.size() > 2)
        satisfactory.log3 = satLogs[2].as<String>();

      // Clear framebuffer and redraw everything
      memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

      // Draw all sections
      drawHeader();
      drawSystemStats(cpuTemp, memUsage);

      // Screen is 960px wide, divide into 3 columns of ~305px each with gaps
      int colWidth = 305;
      int startY = 60;
      int boxHeight = 470;

      drawServerBlock("MC BINGO", bingo, 20, startY, colWidth, boxHeight, true);
      drawServerBlock("MINECRAFT", minecraft, 335, startY, colWidth, boxHeight, true);
      drawServerBlock("SATISFACTORY", satisfactory, 650, startY, colWidth, boxHeight, false);

      // Update the display
      updateDisplay();

      Serial.println("Display updated");
    }
    else
    {
      Serial.print("JSON parse error: ");
      Serial.println(error.c_str());
    }
  }
  else
  {
    Serial.print("HTTP error: ");
    Serial.println(httpCode);
  }

  http.end();
}

// ============================================================================
// Setup & Loop
// ============================================================================

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========================================");
  Serial.println("Docker Game Server Monitor");
  Serial.println("========================================\n");

  // Allocate framebuffer
  Serial.println("Allocating framebuffer...");
  framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
  if (!framebuffer)
  {
    Serial.println("ERROR: Framebuffer allocation failed!");
    while (1)
      delay(1000);
  }
  Serial.println("Framebuffer OK");

  // Initialize display
  Serial.println("Initializing display...");
  epd_init();
  clearDisplay();
  Serial.println("Display OK");

  // Connect to WiFi
  if (!connectToWiFi())
  {
    Serial.println("Cannot continue without WiFi");
    while (1)
      delay(1000);
  }

  // First update
  Serial.println("\nFetching initial data...");
  fetchAndDisplayData(true);

  lastUpdate = millis();
  firstUpdate = false;

  Serial.println("\nMonitoring started!");
  Serial.println("Update interval: 5 seconds\n");
}

void loop()
{
  unsigned long currentTime = millis();

  // Check WiFi
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi lost, reconnecting...");
    connectToWiFi();
  }

  // Update every 5 seconds
  if (currentTime - lastUpdate >= UPDATE_INTERVAL)
  {
    lastUpdate = currentTime;
    fetchAndDisplayData(false);

    Serial.print("Free heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.print(" | PSRAM: ");
    Serial.println(ESP.getFreePsram());
  }

  delay(100);
}
