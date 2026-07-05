#ifndef SENSOR_DRIVER_H
#define SENSOR_DRIVER_H

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <RTClib.h>
#include <Adafruit_HTU21DF.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_SHT31.h>
#include <PMS.h>
#include <SdsDustSensor.h>
#include "config.h"
#include "types.h"

// ============ CÁC THÀNH PHẦN CỨNG ============
// Khởi tạo một lần, giữ ở đây để khỏi lăn tăn global
extern RTC_DS3231 g_rtc;
extern Adafruit_BME280 g_bme;
extern Adafruit_BMP280 g_bmp;
extern Adafruit_HTU21DF g_htu;
extern Adafruit_SHT31 g_sht;
extern Adafruit_AHTX0 g_aht;
extern PMS g_pms;
extern PMS::DATA g_pms_data;
extern SdsDustSensor g_sds;

// ============ HÀM DRIVER CẢM BIẾN ============
// Chỉ đọc giá trị từ cảm biến
// Không kiểm tra hay xử lý lỗi, việc đó để service làm

namespace SensorDriver {

  // Khởi tạo toàn bộ phần cứng cảm biến
  Status_t init();
  
  // ---- I2C Sensors ----
  
  // BME280: trả về nhiệt độ, độ ẩm, áp suất
  Status_t readBME280(float& temp, float& humidity, float& pressure);
  
  // BMP280: trả về nhiệt độ và áp suất
  Status_t readBMP280(float& temp, float& pressure);
  
  // HTU21D: trả về nhiệt độ và độ ẩm
  Status_t readHTU21D(float& temp, float& humidity);
  
  // SHT31: trả về nhiệt độ và độ ẩm
  Status_t readSHT31(float& temp, float& humidity);
  
  // AHT20: trả về nhiệt độ và độ ẩm
  Status_t readAHT20(float& temp, float& humidity);
  
  // ---- UART Sensors ----
  
  // PMS7003: điền dữ liệu vào struct
  Status_t readPMS7003(int& pm1, int& pm25, int& pm10);
  
  // SDS011: returns PM2.5 and PM10
  Status_t readSDS011(float& pm25, float& pm10);
  
  // ---- RTC ----
  
  // DS3231: returns current date/time
  Status_t readRTC(DateTime& now);
  
  // Set RTC time (Unix timestamp)
  Status_t setRTC(uint32_t unix_timestamp);
  
  // Kiểm tra nếu RTC đang ở trạng thái pin yếu (battery low)
  uint8_t isRTCBatteryLow();
  
  // Tái khởi tạo cảm biến nếu cần
  Status_t reinitBME280();
  Status_t reinitBMP280();
  Status_t reinitHTU21D();
  Status_t reinitSHT31();
  Status_t reinitAHT20();
  Status_t reinitPMS7003();
  Status_t reinitSDS011();

}

#endif // SENSOR_DRIVER_H
