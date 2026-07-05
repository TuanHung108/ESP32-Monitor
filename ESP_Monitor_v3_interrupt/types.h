#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

// ============ CẤU TRÚC DỮ LIỆU CẢM BIẾN ============
// Gom tất cả dữ liệu cảm biến vào một struct
// Giảm số biến toàn cục đáng kể
typedef struct {
  // PMS7003 (particle matter)
  int pm1_0;      // PM1.0 in µg/m³
  int pm2_5;      // PM2.5 in µg/m³
  int pm10_0;     // PM10 in µg/m³
  
  // SDS011 (dust)
  float sds_pm25; // PM2.5 in µg/m³
  float sds_pm10; // PM10 in µg/m³
  
  // BME280 (temp/humid/pressure)
  float bme_temp; // °C
  float bme_humi; // %
  float bme_pres; // hPa
  
  // BMP280 (temp/pressure)
  float bmp_temp; // °C
  float bmp_pres; // hPa
  
  // HTU21D (temp/humid)
  float htu_temp; // °C
  float htu_humi; // %
  
  // SHT31 (temp/humid)
  float sht_temp; // °C
  float sht_humi; // %
  
  // AHT20 (temp/humid)
  float aht_temp; // °C
  float aht_humi; // %
  
  // Timestamp
  uint32_t timestamp; // Unix timestamp from RTC
  char date_str[11];  // "YYYY/MM/DD"
  char time_str[9];   // "HH:MM:SS"
  
  // Flags
  uint8_t valid_mask; // Bitmask: bit=1 sensor valid, bit=0 sensor error
  uint8_t sd_ok;      // 1 = SD card ready, 0 = SD error
  float sd_total_mb;  // SD card total size in MB
  float sd_used_mb;   // SD card used size in MB
  float sd_free_mb;   // SD card free size in MB
  float sd_percent;   // SD card usage percentage

} SensorData_t;

// Bit cho valid_mask
#define VALID_PMS     (1 << 0)
#define VALID_SDS     (1 << 1)
#define VALID_BME     (1 << 2)
#define VALID_BMP     (1 << 3)
#define VALID_HTU     (1 << 4)
#define VALID_SHT     (1 << 5)
#define VALID_AHT     (1 << 6)
#define VALID_RTC     (1 << 7)

// ============ CẤU TRÚC TRẠNG THÁI HỆ THỐNG ============
// Theo dõi trạng thái chung của hệ thống
typedef struct {
  uint8_t sd_ok;           // 1 = SD card working, 0 = error
  uint8_t wifi_connected;  // 1 = WiFi connected, 0 = disconnected
  uint8_t ntp_synced;      // 1 = NTP time synced at least once
  uint32_t uptime_ms;      // System uptime in milliseconds
  uint32_t sample_count;   // Number of samples taken
  uint8_t oled_page;       // Current OLED page (0-2)
  
} SystemState_t;

// ============ CẤU TRÚC LOG ============
// Dùng cho logger vòng
typedef struct {
  uint32_t timestamp;      // Unix timestamp
  uint8_t level;           // 0=INFO, 1=WARN, 2=ERROR
  char tag[16];            // Source tag (e.g., "SensorService")
  char message[64];        // Log message
  
} LogEntry_t;

// Mức log
#define LOG_LEVEL_INFO  0
#define LOG_LEVEL_WARN  1
#define LOG_LEVEL_ERROR 2

// ============ TRẠNG THÁI TRẢ VỀ ============
typedef enum {
  STATUS_OK          = 0,
  STATUS_ERROR       = 1,
  STATUS_TIMEOUT     = 2,
  STATUS_INVALID     = 3,
  STATUS_NOT_READY   = 4,
  
} Status_t;

#endif // TYPES_H
