// Implementation of system_manager.h
// Coordinates all services and manages main control flow

#include "system_manager.h"
#include "storage_driver.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include <stdarg.h>

static Adafruit_SSD1306 g_display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

static Status_t _initOLED();
static void _oledClear();
static void _oledPrintln(const char* text);
static void _oledPrintf(const char* fmt, ...);
static void _oledUpdate();
static void _oledSetCursor(uint8_t x, uint8_t y);
static void _oledDrawHLine(uint8_t x, uint8_t y, uint8_t len);
static void _updateOLED();

// Global logger instance
Logger g_logger;

// System state
static SystemState_t s_system_state = {0};
static SensorData_t s_latest_data = {0};
static uint32_t s_last_sample_ms = 0;
static uint32_t s_last_csv_log_ms = 0;
static uint32_t s_last_oled_update_ms = 0;
static uint8_t s_oled_page = 0;

namespace SystemManager {

  Status_t init() {
    // Initialize Serial first (for logging)
    Serial.begin(SERIAL_BAUD);
    delay(500);
    
    g_logger.init();
    g_logger.info("SysMgr", "Boot starting...");
    
    // Initialize I2C
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(I2C_CLOCK);
    delay(200);
    
    // Initialize UART sensors
    Serial2.begin(PMS_BAUD, SERIAL_8N1, PMS_RX_PIN, PMS_TX_PIN);
    Serial1.begin(SDS_BAUD, SERIAL_8N1, SDS_RX_PIN, SDS_TX_PIN);
    
    // Initialize display
    if (_initOLED() != STATUS_OK) {
      g_logger.error("SysMgr", "OLED init failed");
    } else {
      _oledClear();
      _oledPrintln("Initializing...");
      _oledUpdate();
    }
    
    // Initialize storage
    if (StorageDriver::init() != STATUS_OK) {
      g_logger.error("SysMgr", "SD init failed");
      s_system_state.sd_ok = 0;
    } else {
      s_system_state.sd_ok = 1;
      // Write CSV header if file doesn't exist
      File csv_file = StorageDriver::openCSVFile(CSV_FILENAME);
      if (csv_file) {
        if (!StorageDriver::fileExists(CSV_FILENAME) || csv_file.size() == 0) {
          // Write BOM for UTF-8
          csv_file.write(0xEF);
          csv_file.write(0xBB);
          csv_file.write(0xBF);
          // Write header EXACTLY ONCE and keep it intact (no extra spaces/newlines)
          // Use write() instead of writeFileLine() to ensure byte-for-byte header string.
          csv_file.write((const uint8_t*)CSV_HEADER, strlen(CSV_HEADER));
          // Ensure trailing newline so the first data line starts on a new row
          csv_file.write((uint8_t)'\n');
        }
        StorageDriver::closeFile(csv_file);
      }
    }
    
    // Initialize time service
    if (TimeService::init() != STATUS_OK) {
      g_logger.error("SysMgr", "Time init failed");
    }
    
    // Initialize sensor service
    if (SensorService::init() != STATUS_OK) {
      g_logger.error("SysMgr", "Sensor init failed");
    }
    
    // Initialize web service
    WebService::init();
    
    // Initialize WiFi (simplified - copied from v5)
    WiFi.config(STATIC_IP, GATEWAY_IP, SUBNET_MASK, DNS_IP);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    uint32_t wifi_start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - wifi_start < 10000) {
      delay(500);
      Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      s_system_state.wifi_connected = 1;
      g_logger.info("SysMgr", "WiFi connected");
      Serial.println("\nWiFi IP: " + WiFi.localIP().toString());
    } else {
      s_system_state.wifi_connected = 0;
      g_logger.warn("SysMgr", "WiFi connection failed");
    }
    
    // Register web handlers
    g_server.on("/", WebService::handleRoot);
    g_server.on("/live-data", WebService::handleLiveJSON);
    g_server.on("/history-data", WebService::handleHistoryJSON);
    g_server.on("/days", WebService::handleDaysJSON);
    g_server.on("/download", WebService::handleDownloadCSV);
    g_server.on("/delete", WebService::handleDeleteData);
    g_server.on("/settime", WebService::handleSetTime);
    
    // Start web server
    WebService::start();
    
    // Initialize watchdog timer (30 second timeout)
    if (ENABLE_WATCHDOG) {
      esp_task_wdt_config_t wdt_config = {
          .timeout_ms = WATCHDOG_TIMEOUT_S * 1000,
          .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
          .trigger_panic = true
      };

      esp_task_wdt_init(&wdt_config);
      esp_task_wdt_add(nullptr);
      g_logger.info("SysMgr", "Watchdog enabled");
    }
    
    g_logger.info("SysMgr", "Boot complete");
    
    return STATUS_OK;
  }
  
  uint32_t update() {
    // Reset watchdog
    if (ENABLE_WATCHDOG) {
      esp_task_wdt_reset();
    }
    
    uint32_t now_ms = millis();
    s_system_state.uptime_ms = now_ms;

    // Poll PMS7003 frequently enough to keep the UART buffer drained
    SensorService::pollPMS7003();
    
    // ---- SENSOR READING (every 5 seconds) ----
    if (now_ms - s_last_sample_ms >= SAMPLE_INTERVAL_MS) {
      s_last_sample_ms = now_ms;
      s_system_state.sample_count++;

      // Read all sensors (attempt read regardless of individual sensor failures)
      Status_t r = SensorService::readAllSensors(s_latest_data);

      // Refresh SD status each cycle so UI reflects real state
      s_system_state.sd_ok = StorageDriver::isReady() ? 1 : 0;
      s_latest_data.sd_ok = s_system_state.sd_ok;

      float totalMB = 0, usedMB = 0, freeMB = 0;
      StorageDriver::getStats(totalMB, usedMB, freeMB);
      s_latest_data.sd_total_mb = totalMB;
      s_latest_data.sd_used_mb = usedMB;
      s_latest_data.sd_free_mb = freeMB;
      s_latest_data.sd_percent = (totalMB > 0) ? (usedMB / totalMB * 100.0f) : 0.0f;

      // Always push latest (partial or full) data to web service for realtime UI
      WebService::setCurrentSensorData(s_latest_data);

      // If a full read succeeded, write CSV (only when SD ready)
      if (r == STATUS_OK) {
        if (s_system_state.sd_ok && now_ms - s_last_csv_log_ms >= LOG_CSV_INTERVAL_MS) {
          s_last_csv_log_ms = now_ms;

          static char csv_line[256];
          int len = DataService::formatCSVLine(s_latest_data, csv_line, sizeof(csv_line));

          if (len > 0) {
            File csv_file = StorageDriver::openCSVFile(CSV_FILENAME);
            if (csv_file) {
              StorageDriver::writeFileLine(csv_file, csv_line);
              StorageDriver::closeFile(csv_file);
            }
          }
        }
      }
    }
    
    // ---- TIME SYNC (NTP) ----
    if (s_system_state.wifi_connected && TimeService::shouldSyncNTP()) {
      TimeService::syncWithNTP();
      s_system_state.ntp_synced = TimeService::isNTPSynced() ? 1 : 0;
    }
    
    // ---- WEB SERVER ----
    WebService::handleClient();
    
    // ---- OLED UPDATE ----
    if (now_ms - s_last_oled_update_ms >= SAMPLE_INTERVAL_MS) {
      s_last_oled_update_ms = now_ms;
      _updateOLED();
      s_system_state.oled_page = s_oled_page;
    }
    
    return 100;  // Update again in 100ms
  }
  
  const SystemState_t& getSystemState() {
    return s_system_state;
  }
  
  const SensorData_t& getLatestSensorData() {
    return s_latest_data;
  }
  
  Status_t setTimeFromWeb(uint32_t unix_timestamp) {
    return TimeService::setTimeFromWeb(unix_timestamp);
  }
  
  Status_t triggerNTPSync() {
    if (s_system_state.wifi_connected) {
      return TimeService::syncWithNTP();
    }
    return STATUS_ERROR;
  }
  
  uint32_t getUptimeMS() {
    return s_system_state.uptime_ms;
  }
  
  void resetWatchdog() {
    if (ENABLE_WATCHDOG) {
      esp_task_wdt_reset();
    }
  }
  
  void logSystemStats() {
    g_logger.info("SysMgr", "--- SYSTEM STATS ---");
    char buf[64];
    snprintf(buf, sizeof(buf), "Uptime: %u ms", s_system_state.uptime_ms);
    g_logger.info("SysMgr", buf);
    snprintf(buf, sizeof(buf), "Samples: %u", s_system_state.sample_count);
    g_logger.info("SysMgr", buf);
    g_logger.printAll();
  }
  
  // ============ PRIVATE FUNCTIONS ============
  // OLED helper implementation is defined after SystemManager namespace.

}

static void _updateOLED() {
  _oledClear();
  _oledSetCursor(0, 0);
  char line[32];
  snprintf(line, sizeof(line), "Date: %s", s_latest_data.date_str);
  _oledPrintln(line);
  snprintf(line, sizeof(line), "Time: %s", s_latest_data.time_str);
  _oledPrintln(line);
  _oledDrawHLine(0, 18, 128);
  _oledSetCursor(0, 22);
  
  if (s_oled_page == 0) {
    _oledPrintln("[P1] Dust");
    _oledPrintf("PMS25:%d SDS:%.1f", s_latest_data.pm2_5, s_latest_data.sds_pm25);
    _oledPrintf("PMS10:%d SDS:%.1f", s_latest_data.pm10_0, s_latest_data.sds_pm10);
  } else if (s_oled_page == 1) {
    _oledPrintln("[P2] BME/BMP");
    _oledPrintf("BME T:%.1fC H:%.0f%%", s_latest_data.bme_temp, s_latest_data.bme_humi);
    _oledPrintf("BMP T:%.1fC P:%.0f", s_latest_data.bmp_temp, s_latest_data.bmp_pres);
  } else if (s_oled_page == 2) {
    _oledPrintln("[P3] Env");
    _oledPrintf("HTU:%.1fC AHT:%.1fC", s_latest_data.htu_temp, s_latest_data.aht_temp);
    _oledPrintf("SHT:%.1fC H:%.0f%%", s_latest_data.sht_temp, s_latest_data.sht_humi);
  }
  
  _oledUpdate();
  s_oled_page = (s_oled_page + 1) % 3;
}

static Status_t _initOLED() {
  if (!g_display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    return STATUS_ERROR;
  }

  g_display.clearDisplay();
  g_display.setTextSize(1);
  g_display.setTextColor(WHITE);
  g_display.setCursor(0, 0);
  g_display.println("System Boot...");
  g_display.display();
  return STATUS_OK;
}

static void _oledClear() {
  g_display.clearDisplay();
}

static void _oledPrintln(const char* text) {
  g_display.println(text);
}

static void _oledPrintf(const char* fmt, ...) {
  char buffer[128];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  g_display.println(buffer);
}

static void _oledUpdate() {
  g_display.display();
}

static void _oledSetCursor(uint8_t x, uint8_t y) {
  g_display.setCursor(x, y);
}

static void _oledDrawHLine(uint8_t x, uint8_t y, uint8_t len) {
  g_display.drawLine(x, y, x + len, y, WHITE);
}
