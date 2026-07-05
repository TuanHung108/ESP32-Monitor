// Implement hệ thống quản lý chính
// Điều phối các service và vòng lặp chính

#include "system_manager.h"
#include "storage_driver.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include <stdarg.h>

static Adafruit_SSD1306 g_display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

static hw_timer_t* g_sample_timer = nullptr;
static volatile bool g_sample_flag = false;
static portMUX_TYPE g_sample_mux = portMUX_INITIALIZER_UNLOCKED;

static IRAM_ATTR void _onSampleTimerISR() {
  portENTER_CRITICAL_ISR(&g_sample_mux);
  g_sample_flag = true;
  portEXIT_CRITICAL_ISR(&g_sample_mux);
}

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
static uint8_t s_oled_page = 0;

namespace SystemManager {

  Status_t init() {
    // Khởi tạo serial trước để log
    Serial.begin(SERIAL_BAUD);
    uint32_t serial_ready_ms = millis();
    while (millis() - serial_ready_ms < 500) {
      yield();
    }
    
    g_logger.init();
    g_logger.info("SysMgr", "Boot starting...");
    
    // Khởi tạo I2C
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(I2C_CLOCK);
    uint32_t i2c_ready_ms = millis();
    while (millis() - i2c_ready_ms < 200) {
      yield();
    }
    
    // Khởi tạo UART cho cảm biến
    Serial2.begin(PMS_BAUD, SERIAL_8N1, PMS_RX_PIN, PMS_TX_PIN);
    Serial1.begin(SDS_BAUD, SERIAL_8N1, SDS_RX_PIN, SDS_TX_PIN);
    
    // Khởi tạo màn hình OLED
    if (_initOLED() != STATUS_OK) {
      g_logger.error("SysMgr", "OLED init failed");
    } else {
      _oledClear();
      _oledPrintln("Initializing...");
      _oledUpdate();
    }
    
    // Khởi tạo thẻ SD
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
    
    // Khởi tạo service thời gian
    if (TimeService::init() != STATUS_OK) {
      g_logger.error("SysMgr", "Time init failed");
    }
    
    // Khởi tạo service cảm biến
    if (SensorService::init() != STATUS_OK) {
      g_logger.error("SysMgr", "Sensor init failed");
    }
    
    // Khởi tạo web service
    WebService::init();
    
    // Khởi động WiFi
    WiFi.config(STATIC_IP, GATEWAY_IP, SUBNET_MASK, DNS_IP);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    uint32_t wifi_start = millis();
    uint32_t last_wifi_dot_ms = 0;
    while (WiFi.status() != WL_CONNECTED && millis() - wifi_start < 10000) {
      if (millis() - last_wifi_dot_ms >= 500) {
        Serial.print(".");
        last_wifi_dot_ms = millis();
      }
      yield();
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      s_system_state.wifi_connected = 1;
      g_logger.info("SysMgr", "WiFi connected");
      Serial.println("\nWiFi IP: " + WiFi.localIP().toString());
    } else {
      s_system_state.wifi_connected = 0;
      g_logger.warn("SysMgr", "WiFi connection failed");
    }
    
    // Đăng ký các route web
    g_server.on("/", WebService::handleRoot);
    g_server.on("/live-data", WebService::handleLiveJSON);
    g_server.on("/history-data", WebService::handleHistoryJSON);
    g_server.on("/days", WebService::handleDaysJSON);
    g_server.on("/download", WebService::handleDownloadCSV);
    g_server.on("/delete", WebService::handleDeleteData);
    g_server.on("/settime", WebService::handleSetTime);
    
    // Khởi động web server
    WebService::start();

    // Dùng timer phần cứng để tạo tick 5 giây
    g_sample_timer = timerBegin(1000000);  // 1 MHz tick source
    timerAttachInterrupt(g_sample_timer, &_onSampleTimerISR);
    timerAlarm(g_sample_timer, SAMPLE_INTERVAL_MS * 1000, true, 0);
    
    // Khởi tạo watchdog 30 giây
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

    // Đọc PMS7003 thường xuyên để giữ buffer UART sạch
    SensorService::pollPMS7003();
    
    // ---- ĐỌC CẢM BIẾN (được kích hoạt bởi timer) ----
    bool sample_due = false;
    portENTER_CRITICAL(&g_sample_mux);
    sample_due = g_sample_flag;
    g_sample_flag = false;
    portEXIT_CRITICAL(&g_sample_mux);

    if (sample_due) {
      s_last_sample_ms = now_ms;
      s_system_state.sample_count++;

      // Đọc tất cả cảm biến, dù có cảm biến nào lỗi
      Status_t r = SensorService::readAllSensors(s_latest_data);

      // Cập nhật trạng thái SD mỗi chu kỳ để UI đúng
      s_system_state.sd_ok = StorageDriver::isReady() ? 1 : 0;
      s_latest_data.sd_ok = s_system_state.sd_ok;

      float totalMB = 0, usedMB = 0, freeMB = 0;
      StorageDriver::getStats(totalMB, usedMB, freeMB);
      s_latest_data.sd_total_mb = totalMB;
      s_latest_data.sd_used_mb = usedMB;
      s_latest_data.sd_free_mb = freeMB;
      s_latest_data.sd_percent = (totalMB > 0) ? (usedMB / totalMB * 100.0f) : 0.0f;

      // Luôn đẩy dữ liệu mới nhất lên web để dashboard cập nhật
      WebService::setCurrentSensorData(s_latest_data);

      // Nếu đọc xong thì ghi CSV nếu SD sẵn sàng
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

      // Cập nhật OLED theo cùng chu kỳ 5 giây
      _updateOLED();
      s_system_state.oled_page = s_oled_page;
    }
    
    // ---- ĐỒNG BỘ THỜI GIAN (NTP) ----
    if (s_system_state.wifi_connected && TimeService::shouldSyncNTP()) {
      TimeService::syncWithNTP();
      s_system_state.ntp_synced = TimeService::isNTPSynced() ? 1 : 0;
    }
    
    // ---- WEB SERVER ----
    WebService::handleClient();
    
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
  
  // ============ HÀM RIÊNG ============
  // Các hàm hỗ trợ OLED ở phía dưới namespace

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
