// Implement driver thấp cho cảm biến
// Raw sensor I/O operations (no validation, no error recovery)

#include "sensor_driver.h"

// ============ HARDWARE INSTANCES ============
RTC_DS3231 g_rtc;
Adafruit_BME280 g_bme;
Adafruit_BMP280 g_bmp;
Adafruit_HTU21DF g_htu;
Adafruit_SHT31 g_sht;
Adafruit_AHTX0 g_aht;
PMS g_pms(Serial2);
PMS::DATA g_pms_data;
SdsDustSensor g_sds(Serial1);

namespace SensorDriver {

  // ============ INITIALIZATION ============
  
  Status_t init() {
    // Wire (I2C) already initialized in main setup()
    // Serial2 (PMS) already initialized in main setup()
    // Serial1 (SDS) already initialized in main setup()
    
    // Initialize sensors
    if (!g_rtc.begin()) {
      return STATUS_ERROR;
    }
    if (!g_bme.begin(BME280_ADDR)) {
      return STATUS_ERROR;
    }
    if (!g_bmp.begin(BMP280_ADDR)) {
      return STATUS_ERROR;
    }
    if (!g_htu.begin()) {
      return STATUS_ERROR;
    }
    if (!g_sht.begin(SHT31_ADDR)) {
      return STATUS_ERROR;
    }
    if (!g_aht.begin()) {
      return STATUS_ERROR;
    }
    
    // Initialize UART sensors
    g_pms.wakeUp();  // Wake up PMS7003
    uint32_t pms_wakeup_ms = millis();
    while (millis() - pms_wakeup_ms < 100) {
      yield();
    }
    
    g_sds.begin();
    g_sds.setActiveReportingMode();
    
    return STATUS_OK;
  }
  
  // ============ I2C SENSORS ============
  
  Status_t readBME280(float& temp, float& humidity, float& pressure) {
    temp = g_bme.readTemperature();
    humidity = g_bme.readHumidity();
    pressure = g_bme.readPressure() / 100.0f;
    return STATUS_OK;
  }
  
  Status_t readBMP280(float& temp, float& pressure) {
    temp = g_bmp.readTemperature();
    pressure = g_bmp.readPressure() / 100.0f;
    return STATUS_OK;
  }
  
  Status_t readHTU21D(float& temp, float& humidity) {
    temp = g_htu.readTemperature();
    humidity = g_htu.readHumidity();
    return STATUS_OK;
  }
  
  Status_t readSHT31(float& temp, float& humidity) {
    temp = g_sht.readTemperature();
    humidity = g_sht.readHumidity();
    return STATUS_OK;
  }
  
  Status_t readAHT20(float& temp, float& humidity) {
    sensors_event_t h_event, t_event;
    g_aht.getEvent(&h_event, &t_event);
    temp = t_event.temperature;
    humidity = h_event.relative_humidity;
    return STATUS_OK;
  }
  
  // ============ UART SENSORS ============
  
  Status_t readPMS7003(int& pm1, int& pm25, int& pm10) {
    if (Serial2.available() < 32) {
      return STATUS_TIMEOUT;
    }
    if (g_pms.read(g_pms_data)) {
      pm1 = g_pms_data.PM_AE_UG_1_0;
      pm25 = g_pms_data.PM_AE_UG_2_5;
      pm10 = g_pms_data.PM_AE_UG_10_0;
      return STATUS_OK;
    }
    return STATUS_TIMEOUT;
  }
  
  Status_t readSDS011(float& pm25, float& pm10) {
    PmResult res = g_sds.readPm();
    if (res.isOk()) {
      pm25 = res.pm25;
      pm10 = res.pm10;
      return STATUS_OK;
    }
    return STATUS_TIMEOUT;
  }
  
  // ============ RTC ============
  
  Status_t readRTC(DateTime& now) {
    now = g_rtc.now();
    return STATUS_OK;
  }
  
  Status_t setRTC(uint32_t unix_timestamp) {
    // Convert Unix timestamp to DateTime
    time_t time_val = unix_timestamp;
    struct tm* timeinfo = localtime(&time_val);
    DateTime dt(
      timeinfo->tm_year + 1900,
      timeinfo->tm_mon + 1,
      timeinfo->tm_mday,
      timeinfo->tm_hour,
      timeinfo->tm_min,
      timeinfo->tm_sec
    );
    g_rtc.adjust(dt);
    return STATUS_OK;
  }
  
  uint8_t isRTCBatteryLow() {
    return g_rtc.lostPower() ? 1 : 0;
  }
  
  // ============ REINIT (ERROR RECOVERY) ============
  
  Status_t reinitBME280() {
    return g_bme.begin(BME280_ADDR) ? STATUS_OK : STATUS_ERROR;
  }
  
  Status_t reinitBMP280() {
    return g_bmp.begin(BMP280_ADDR) ? STATUS_OK : STATUS_ERROR;
  }
  
  Status_t reinitHTU21D() {
    return g_htu.begin() ? STATUS_OK : STATUS_ERROR;
  }
  
  Status_t reinitSHT31() {
    return g_sht.begin(SHT31_ADDR) ? STATUS_OK : STATUS_ERROR;
  }
  
  Status_t reinitAHT20() {
    return g_aht.begin() ? STATUS_OK : STATUS_ERROR;
  }
  
  Status_t reinitPMS7003() {
    // PMS doesn't need explicit reinit, just read next cycle
    return STATUS_OK;
  }
  
  Status_t reinitSDS011() {
    g_sds.begin();
    g_sds.setActiveReportingMode();
    return STATUS_OK;
  }

}
