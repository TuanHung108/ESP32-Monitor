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

// ============ HARDWARE INSTANCES ============
// Initialized once, kept here instead of scattered globals
extern RTC_DS3231 g_rtc;
extern Adafruit_BME280 g_bme;
extern Adafruit_BMP280 g_bmp;
extern Adafruit_HTU21DF g_htu;
extern Adafruit_SHT31 g_sht;
extern Adafruit_AHTX0 g_aht;
extern PMS g_pms;
extern PMS::DATA g_pms_data;
extern SdsDustSensor g_sds;

// ============ SENSOR DRIVER FUNCTIONS ============
// These are pure I/O - read value from sensor, return raw result
// No validation, no error recovery - that's Service layer's job

namespace SensorDriver {

  // Initialize all sensor hardware
  Status_t init();
  
  // ---- I2C Sensors ----
  
  // BME280: returns temp in °C, humidity in %, pressure in Pa
  Status_t readBME280(float& temp, float& humidity, float& pressure);
  
  // BMP280: returns temp in °C, pressure in Pa
  Status_t readBMP280(float& temp, float& pressure);
  
  // HTU21D: returns temp in °C, humidity in %
  Status_t readHTU21D(float& temp, float& humidity);
  
  // SHT31: returns temp in °C, humidity in %
  Status_t readSHT31(float& temp, float& humidity);
  
  // AHT20: returns temp in °C, humidity in %
  Status_t readAHT20(float& temp, float& humidity);
  
  // ---- UART Sensors ----
  
  // PMS7003: populates data struct
  Status_t readPMS7003(int& pm1, int& pm25, int& pm10);
  
  // SDS011: returns PM2.5 and PM10
  Status_t readSDS011(float& pm25, float& pm10);
  
  // ---- RTC ----
  
  // DS3231: returns current date/time
  Status_t readRTC(DateTime& now);
  
  // Set RTC time (Unix timestamp)
  Status_t setRTC(uint32_t unix_timestamp);
  
  // Check if RTC battery is low
  uint8_t isRTCBatteryLow();
  
  // ---- Reinitialize ----
  // Used for error recovery
  Status_t reinitBME280();
  Status_t reinitBMP280();
  Status_t reinitHTU21D();
  Status_t reinitSHT31();
  Status_t reinitAHT20();
  Status_t reinitPMS7003();
  Status_t reinitSDS011();

}

#endif // SENSOR_DRIVER_H
