// Implement web server và endpoint
// HTTP request handlers and web server management

#include "web_service.h"
#include "storage_driver.h"
#include "time_service.h"
#include "web_pages.h"
#include "logger.h"
#include <WiFi.h>
#include <SD.h>
#include <esp_task_wdt.h>

WebServer g_server(WEB_SERVER_PORT);

// Current sensor data for broadcasting
static SensorData_t s_current_data = {0};

namespace WebService {

  Status_t init() {
    // Routes are registered in main file
    // This just prepares the service
    g_logger.info("WebSvc", "Initialized");
    return STATUS_OK;
  }
  
  void start() {
    g_server.begin();
    g_logger.info("WebSvc", "Server started");
  }
  
  void handleClient() {
    g_server.handleClient();
  }
  
  void setCurrentSensorData(const SensorData_t& data) {
    memcpy(&s_current_data, &data, sizeof(SensorData_t));
  }
  
  const SensorData_t& getCurrentSensorData() {
    return s_current_data;
  }
  
  // ============ HTTP HANDLERS ============
  
  void handleRoot() {
    float totalMB, usedMB, freeMB;
    StorageDriver::getStats(totalMB, usedMB, freeMB);
    float percentUsed = (totalMB > 0) ? (usedMB / totalMB * 100.0f) : 0;
    
    // Pass to HTML generator (from web_pages.h)
    g_server.send(200, "text/html", getDashboardPage(totalMB, usedMB, freeMB, percentUsed));
  }
  
  void handleLiveJSON() {
    static char json_buf[512];
    
    int len = DataService::formatLiveJSON(s_current_data, json_buf, sizeof(json_buf));
    if (len > 0) {
      g_server.send(200, "application/json", json_buf);
    } else {
      g_server.send(500, "text/plain", "Error formatting JSON");
    }
  }
  
  void handleHistoryJSON() {
    String reqDate = "";
    if (g_server.hasArg("date")) {
      reqDate = g_server.arg("date");
      reqDate.replace('-', '/');
    }
    
    if (reqDate.length() == 0) {
      g_server.send(200, "application/json", 
        "{\"timestamps\":[],\"pms25\":[],\"temp\":[],\"hum\":[],\"press\":[]}");
      return;
    }
    
    File csv_file = SD.open(CSV_FILENAME, FILE_READ);
    if (!csv_file) {
      g_server.send(200, "application/json", 
        "{\"timestamps\":[],\"pms25\":[],\"temp\":[],\"hum\":[],\"press\":[]}");
      return;
    }
    
    static char times[144][9];
    static float pm25_vals[144];
    static float temps[144];
    static float humid[144];
    static float press[144];
    char line[256];
    bool first_line = true;
    uint16_t count = 0;

    while (csv_file.available() && count < 144) {
      size_t len = csv_file.readBytesUntil('\n', line, sizeof(line));
      if (len == 0) continue;
      if (line[len - 1] == '\r') {
        line[len - 1] = '\0';
      } else {
        line[len] = '\0';
      }
      if (first_line) {
        first_line = false;
        continue;
      }
      if (line[0] == '\0') continue;
      const char* semi1 = strchr(line, ';');
      if (!semi1) continue;
      char date_str[11];
      uint16_t date_len = semi1 - line;
      if (date_len > 10) date_len = 10;
      strncpy(date_str, line, date_len);
      date_str[date_len] = '\0';
      if (strcmp(date_str, reqDate.c_str()) != 0) continue;
      const char* semi2 = strchr(semi1 + 1, ';');
      const char* semi3 = semi2 ? strchr(semi2 + 1, ';') : nullptr;
      const char* semi4 = semi3 ? strchr(semi3 + 1, ';') : nullptr;
      const char* semi5 = semi4 ? strchr(semi4 + 1, ';') : nullptr;
      const char* semi6 = semi5 ? strchr(semi5 + 1, ';') : nullptr;
      const char* semi7 = semi6 ? strchr(semi6 + 1, ';') : nullptr;
      const char* semi8 = semi7 ? strchr(semi7 + 1, ';') : nullptr;
      if (!semi2 || !semi3 || !semi4 || !semi5 || !semi6 || !semi7 || !semi8) continue;
      strncpy(times[count], semi1 + 1, 8);
      times[count][8] = '\0';
      pm25_vals[count] = atof(semi3 + 1);
      temps[count] = atof(semi7 + 1);
      const char* semi9 = strchr(semi8 + 1, ';');
      if (!semi9) continue;
      humid[count] = atof(semi8 + 1);
      const char* semi10 = strchr(semi9 + 1, ';');
      if (!semi10) continue;
      press[count] = atof(semi9 + 1);
      count++;
    }
    csv_file.close();
    
    if (count == 0) {
      g_server.send(200, "application/json", 
        "{\"timestamps\":[],\"pms25\":[],\"temp\":[],\"hum\":[],\"press\":[]}");
      return;
    }
    
    uint16_t interval = DataService::calculateDownsampleInterval(count);
    static char times_ds[144][9];
    static float pm25_ds[144];
    static float temps_ds[144];
    static float humid_ds[144];
    static float press_ds[144];
    uint16_t ds_count = 0;
    for (uint16_t i = 0; i < count; i++) {
      if (interval > 1 && i % interval != 0 && i != count - 1) continue;
      strcpy(times_ds[ds_count], times[i]);
      pm25_ds[ds_count] = pm25_vals[i];
      temps_ds[ds_count] = temps[i];
      humid_ds[ds_count] = humid[i];
      press_ds[ds_count] = press[i];
      ds_count++;
    }
    
    static char json_buf[8192];
    int len = DataService::formatHistoryJSON(
      times_ds, pm25_ds, temps_ds, humid_ds, press_ds,
      ds_count, json_buf, sizeof(json_buf)
    );
    
    if (len > 0) {
      g_server.send(200, "application/json", json_buf);
    } else {
      g_server.send(500, "text/plain", "Error formatting history");
    }
  }
  
  void handleDaysJSON() {
    File csv_file = SD.open(CSV_FILENAME, FILE_READ);
    if (!csv_file) {
      g_server.send(200, "application/json", "{\"dates\":[]}");
      return;
    }
    
    static char dates[16][11];
    char prev_date[11] = {0};
    char line[256];
    bool first_line = true;
    uint16_t unique_count = 0;

    while (csv_file.available()) {
      size_t len = csv_file.readBytesUntil('\n', line, sizeof(line));
      if (len == 0) continue;
      if (line[len - 1] == '\r') {
        line[len - 1] = '\0';
      } else {
        line[len] = '\0';
      }
      if (first_line) {
        first_line = false;
        continue;
      }
      if (line[0] == '\0') continue;
      const char* semi1 = strchr(line, ';');
      if (!semi1) continue;
      uint16_t date_len = semi1 - line;
      if (date_len > 10) date_len = 10;
      char curr_date[11];
      strncpy(curr_date, line, date_len);
      curr_date[date_len] = '\0';

      if (strcmp(curr_date, prev_date) == 0) continue;
      strcpy(prev_date, curr_date);
      if (unique_count < 16) {
        strcpy(dates[unique_count], curr_date);
        unique_count++;
      } else {
        // keep only last 16 date values
        for (uint16_t j = 1; j < 16; j++) {
          strcpy(dates[j - 1], dates[j]);
        }
        strcpy(dates[15], curr_date);
      }
    }
    csv_file.close();
    
    if (unique_count == 0) {
      g_server.send(200, "application/json", "{\"dates\":[]}");
      return;
    }
    
    uint16_t start = (unique_count > 7) ? (unique_count - 7) : 0;
    static char json_buf[512];
    int pos = snprintf(json_buf, sizeof(json_buf), "{\"dates\":[");
    for (uint16_t i = start; i < unique_count; i++) {
      char formatted[11];
      strcpy(formatted, dates[i]);
      for (int j = 0; j < 10; j++) {
        if (formatted[j] == '/') formatted[j] = '-';
      }
      if (i > start) pos += snprintf(json_buf + pos, sizeof(json_buf) - pos, ",");
      pos += snprintf(json_buf + pos, sizeof(json_buf) - pos, "\"%s\"", formatted);
    }
    pos += snprintf(json_buf + pos, sizeof(json_buf) - pos, "]}");
    g_server.send(200, "application/json", json_buf);
  }
  
  void handleDownloadCSV() {
    // Restore safe streaming (no header manipulation):
    // stream raw file directly from SD, so download always contains data.
    // While streaming, periodically handle clients to keep UI alive.

    if (!StorageDriver::fileExists(CSV_FILENAME)) {
      g_server.send(404, "text/plain", "File not found");
      return;
    }

    File file = SD.open(CSV_FILENAME, FILE_READ);
    if (!file) {
      g_server.send(500, "text/plain", "Cannot open file");
      return;
    }

    g_server.sendHeader("Cache-Control", "no-store");
    g_server.sendHeader("Connection", "close");
    g_server.sendHeader("Content-Type", "text/csv");
    g_server.sendHeader("Content-Disposition", "attachment; filename=\"datalog.csv\"");
    g_server.sendHeader("Content-Transfer-Encoding", "binary");

    g_server.setContentLength(file.size());
    g_server.send(200, "text/csv", "");

    static uint8_t buf[1024];
    while (file.available()) {
      size_t toRead = file.available();
      if (toRead > sizeof(buf)) toRead = sizeof(buf);
      size_t n = file.read(buf, toRead);
      if (n == 0) break;
      g_server.client().write(buf, n);

      WebService::handleClient();
      delay(1);
    }

    file.close();
  }
  
  void handleDeleteData() {
    if (!g_server.hasArg("pass") || g_server.arg("pass") != "delete123") {
      g_server.send(403, "text/html", 
        "<script>alert('Sai mat khau!');window.location.href='/';</script>");
      return;
    }
    
    StorageDriver::deleteFile(CSV_FILENAME);
    g_logger.info("WebSvc", "Data deleted");
    
    g_server.send(200, "text/html", 
      "<script>alert('Da xoa thanh cong!');window.location.href='/';</script>");
  }
  
  void handleSetTime() {
    if (g_server.hasArg("t")) {
      uint32_t unix_ts = g_server.arg("t").toInt();
      TimeService::setTimeFromWeb(unix_ts);
      g_server.send(200, "text/plain", "OK");
    } else {
      g_server.send(400, "text/plain", "Missing parameter");
    }
  }

}

