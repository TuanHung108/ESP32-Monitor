#ifndef CONFIG_H
#define CONFIG_H

// ============ CẤU HÌNH PHẦN CỨNG ============

// OLED Display
#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT       64
#define OLED_ADDR           0x3C

// SD Card
#define SD_CS_PIN           5

// I2C
#define I2C_SDA             21
#define I2C_SCL             22
#define I2C_CLOCK           100000

// UART - PMS7003 (Serial2)
#define PMS_RX_PIN          16
#define PMS_TX_PIN          17
#define PMS_BAUD            9600

// UART - SDS011 (Serial1)
#define SDS_RX_PIN          26
#define SDS_TX_PIN          27
#define SDS_BAUD            9600

// Main Serial (Debug/Log)
#define SERIAL_BAUD         115200

// I2C Sensor Addresses
#define BME280_ADDR         0x76
#define BMP280_ADDR         0x77
#define SHT31_ADDR          0x44

// ============ LẤY MẪU VÀ THỜI GIAN ============

// Main sampling interval (5 seconds)
#define SAMPLE_INTERVAL_MS  5000

// Sensor reinit cooldown (30 seconds)
#define REINIT_COOLDOWN_MS  30000

// NTP sync interval (6 hours)
#define NTP_SYNC_INTERVAL   21600000UL

// Warmup delay after boot (5 seconds before first NTP sync)
#define NTP_WARMUP_DELAY    5000

// OLED update interval (each sample triggers OLED)
#define OLED_PAGE_CYCLE     3  // Cycle through 3 pages

// CSV logging interval (same as sample interval)
#define LOG_CSV_INTERVAL_MS SAMPLE_INTERVAL_MS

// ============ GIỚI HẠN VÀ NGƯỠNG ============

// Temperature range validation (°C)
#define TEMP_MIN            -20.0f
#define TEMP_MAX            80.0f

// Humidity range validation (%)
#define HUMI_MIN            0.0f
#define HUMI_MAX            100.0f

// Pressure range validation (hPa)
#define PRESS_MIN           800.0f
#define PRESS_MAX           1200.0f

// Data buffer sizes
#define MAX_CSV_LINE_LEN    256
#define MAX_JSON_LINE_LEN   512

// Logger
#define LOG_BUFFER_SIZE     10  // Number of log entries to keep

// ============ CẤU HÌNH MẠNG ============

// WiFi credentials
#define WIFI_SSID           "BinhHung"
#define WIFI_PASS           "7474747474"

// IP configuration
#define STATIC_IP           IPAddress(192, 168, 1, 200)
#define GATEWAY_IP          IPAddress(192, 168, 1, 1)
#define SUBNET_MASK         IPAddress(255, 255, 255, 0)
#define DNS_IP              IPAddress(8, 8, 8, 8)

// Web server port
#define WEB_SERVER_PORT     80

// NTP servers
#define NTP_SERVER          "pool.ntp.org"
#define TIMEZONE_OFFSET     7  // UTC+7 for Vietnam

// ============ THẺ SD VÀ LƯU TRỮ ============

#define CSV_FILENAME        "/datalog.csv"
#define CSV_BOM_BYTES       3  // UTF-8 BOM: 0xEF, 0xBB, 0xBF

// CSV header (semicolon-separated)
#define CSV_HEADER          "Ngày đo;Giờ đo;PMS_PM1.0 (µg/m³);PMS_PM2.5 (µg/m³);PMS_PM10 (µg/m³);SDS_PM2.5 (µg/m³);SDS_PM10 (µg/m³);BME_Nhiệt (°C);BME_Ẩm (%);BME_Áp suất (hPa);BMP_Nhiệt (°C);BMP_Áp suất (hPa);HTU_Nhiệt (°C);HTU_Ẩm (%);SHT_Nhiệt (°C);SHT_Ẩm (%);AHT_Nhiệt (°C);AHT_Ẩm (%)"

// ============ WATCHDOG ============

#define WATCHDOG_TIMEOUT_S  30  // 30 second watchdog

// ============ CỜ TÍNH NĂNG ============

#define ENABLE_LOGGING      1   // Enable structured logging
#define ENABLE_WATCHDOG     1   // Enable ESP32 watchdog timer
#define ENABLE_DEBUG        1   // Enable debug output to Serial

#endif // CONFIG_H
