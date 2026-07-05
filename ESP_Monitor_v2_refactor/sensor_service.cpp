// Implement service đọc cảm biến
// Đọc và kiểm tra dữ liệu, xử lý lỗi nhẹ

#include "sensor_service.h"
#include "time_service.h"

// Track last reinit time for each sensor (cooldown)
static uint32_t s_last_reinit_bme = 0;
static uint32_t s_last_reinit_bmp = 0;
static uint32_t s_last_reinit_htu = 0;
static uint32_t s_last_reinit_sht = 0;
static uint32_t s_last_reinit_aht = 0;
static uint32_t s_last_reinit_pms = 0;
static uint32_t s_last_reinit_sds = 0;

// PMS7003 cache for fast, frequent UART polling
static int s_last_pms_pm1 = 0;
static int s_last_pms_pm25 = 0;
static int s_last_pms_pm10 = 0;
static bool s_last_pms_valid = false;

// Validation helpers
inline bool _isValidTemperature(float t) {
  return (t > TEMP_MIN && t < TEMP_MAX);
}

inline bool _isValidHumidity(float h) {
  return (h >= HUMI_MIN && h <= HUMI_MAX);
}

inline bool _isValidPressure(float p) {
  return (p > PRESS_MIN && p < PRESS_MAX);
}

namespace SensorService {

  Status_t init() {
    Status_t status = SensorDriver::init();
    if (status != STATUS_OK) {
      g_logger.error("SensorSvc", "Driver init failed");
    }
    return status;
  }
  
  Status_t readAllSensors(SensorData_t& data) {
    data.valid_mask = 0;
    if (TimeService::getCurrentTime(data.date_str, data.time_str, data.timestamp) != STATUS_OK) {
      g_logger.warn("SensorSvc", "Time read failed");
    }
    
    uint32_t now_ms = millis();
    
    // Read PMS7003
    int pm1 = 0, pm25 = 0, pm10 = 0;
    if (s_last_pms_valid) {
      pm1 = s_last_pms_pm1;
      pm25 = s_last_pms_pm25;
      pm10 = s_last_pms_pm10;
      data.pm1_0 = pm1;
      data.pm2_5 = pm25;
      data.pm10_0 = pm10;
      data.valid_mask |= VALID_PMS;
    } else if (_readPMS7003WithRecovery(pm1, pm25, pm10) == STATUS_OK) {
      s_last_pms_valid = true;
      s_last_pms_pm1 = pm1;
      s_last_pms_pm25 = pm25;
      s_last_pms_pm10 = pm10;
      data.pm1_0 = pm1;
      data.pm2_5 = pm25;
      data.pm10_0 = pm10;
      data.valid_mask |= VALID_PMS;
    } else {
      // PMS failed: keep last known PMS readings rather than replacing them with zero.
      g_logger.warn("SensorSvc", "PMS7003 read failed, preserving last value");
    }
    
    // Read SDS011
    float sds_pm25 = 0, sds_pm10 = 0;
    if (_readSDS011WithRecovery(sds_pm25, sds_pm10) == STATUS_OK) {
      data.sds_pm25 = sds_pm25;
      data.sds_pm10 = sds_pm10;
      data.valid_mask |= VALID_SDS;
    } else {
      g_logger.error("SensorSvc", "SDS011 unavailable");
      // Preserve last known SDS values until a new valid reading is available.
    }
    
    // Read BME280
    float bme_t, bme_h, bme_p;
    if (_readBME280WithRecovery(bme_t, bme_h, bme_p, now_ms) == STATUS_OK) {
      data.bme_temp = bme_t;
      data.bme_humi = bme_h;
      data.bme_pres = bme_p;
      data.valid_mask |= VALID_BME;
    }
    
    // Read BMP280
    float bmp_t, bmp_p;
    if (_readBMP280WithRecovery(bmp_t, bmp_p, now_ms) == STATUS_OK) {
      data.bmp_temp = bmp_t;
      data.bmp_pres = bmp_p;
      data.valid_mask |= VALID_BMP;
    }
    
    // Read HTU21D
    float htu_t, htu_h;
    if (_readHTU21DWithRecovery(htu_t, htu_h, now_ms) == STATUS_OK) {
      data.htu_temp = htu_t;
      data.htu_humi = htu_h;
      data.valid_mask |= VALID_HTU;
    }
    
    // Read SHT31
    float sht_t, sht_h;
    if (_readSHT31WithRecovery(sht_t, sht_h, now_ms) == STATUS_OK) {
      data.sht_temp = sht_t;
      data.sht_humi = sht_h;
      data.valid_mask |= VALID_SHT;
    }
    
    // Read AHT20
    float aht_t, aht_h;
    if (_readAHT20WithRecovery(aht_t, aht_h, now_ms) == STATUS_OK) {
      data.aht_temp = aht_t;
      data.aht_humi = aht_h;
      data.valid_mask |= VALID_AHT;
    }
    
    return (data.valid_mask > 0) ? STATUS_OK : STATUS_ERROR;
  }
  
  // ============ RECOVERY FUNCTIONS ============
  // Try read, if fail and cooldown passed, reinit and retry
  
  Status_t _readBME280WithRecovery(float& temp, float& humidity, float& pressure, uint32_t now_ms) {
    Status_t status = SensorDriver::readBME280(temp, humidity, pressure);
    
    if (status != STATUS_OK || 
        !_isValidTemperature(temp) || 
        !_isValidHumidity(humidity) || 
        !_isValidPressure(pressure)) {
      
      if (now_ms - s_last_reinit_bme > REINIT_COOLDOWN_MS) {
        SensorDriver::reinitBME280();
        s_last_reinit_bme = now_ms;
        g_logger.warn("SensorSvc", "BME280 reinit");
      }
      return STATUS_ERROR;
    }
    
    return STATUS_OK;
  }
  
  Status_t _readBMP280WithRecovery(float& temp, float& pressure, uint32_t now_ms) {
    Status_t status = SensorDriver::readBMP280(temp, pressure);
    
    if (status != STATUS_OK || 
        !_isValidTemperature(temp) || 
        !_isValidPressure(pressure)) {
      
      if (now_ms - s_last_reinit_bmp > REINIT_COOLDOWN_MS) {
        SensorDriver::reinitBMP280();
        s_last_reinit_bmp = now_ms;
        g_logger.warn("SensorSvc", "BMP280 reinit");
      }
      return STATUS_ERROR;
    }
    
    return STATUS_OK;
  }
  
  Status_t _readHTU21DWithRecovery(float& temp, float& humidity, uint32_t now_ms) {
    Status_t status = SensorDriver::readHTU21D(temp, humidity);
    
    if (status != STATUS_OK || 
        !_isValidTemperature(temp) || 
        !_isValidHumidity(humidity)) {
      
      if (now_ms - s_last_reinit_htu > REINIT_COOLDOWN_MS) {
        SensorDriver::reinitHTU21D();
        s_last_reinit_htu = now_ms;
        g_logger.warn("SensorSvc", "HTU21D reinit");
      }
      return STATUS_ERROR;
    }
    
    return STATUS_OK;
  }
  
  Status_t _readSHT31WithRecovery(float& temp, float& humidity, uint32_t now_ms) {
    Status_t status = SensorDriver::readSHT31(temp, humidity);
    
    if (status != STATUS_OK || 
        !_isValidTemperature(temp) || 
        !_isValidHumidity(humidity)) {
      
      if (now_ms - s_last_reinit_sht > REINIT_COOLDOWN_MS) {
        SensorDriver::reinitSHT31();
        s_last_reinit_sht = now_ms;
        g_logger.warn("SensorSvc", "SHT31 reinit");
      }
      return STATUS_ERROR;
    }
    
    return STATUS_OK;
  }
  
  Status_t _readAHT20WithRecovery(float& temp, float& humidity, uint32_t now_ms) {
    Status_t status = SensorDriver::readAHT20(temp, humidity);
    
    if (status != STATUS_OK || 
        !_isValidTemperature(temp) || 
        !_isValidHumidity(humidity)) {
      
      if (now_ms - s_last_reinit_aht > REINIT_COOLDOWN_MS) {
        SensorDriver::reinitAHT20();
        s_last_reinit_aht = now_ms;
        g_logger.warn("SensorSvc", "AHT20 reinit");
      }
      return STATUS_ERROR;
    }
    
    return STATUS_OK;
  }
  
  Status_t _readPMS7003WithRecovery(int& pm1, int& pm25, int& pm10) {
    Status_t status = SensorDriver::readPMS7003(pm1, pm25, pm10);
    
    // If read failed, return error (don't use 0 as fallback)
    if (status != STATUS_OK) {
      return STATUS_ERROR;
    }
    
    return STATUS_OK;
  }
  
  Status_t _readSDS011WithRecovery(float& pm25, float& pm10) {
    Status_t status = SensorDriver::readSDS011(pm25, pm10);
    
    // If read failed, return error
    if (status != STATUS_OK) {
      g_logger.warn("SensorSvc", "SDS011 read failed");
      return STATUS_ERROR;
    }
    
    return STATUS_OK;
  }

  Status_t pollPMS7003() {
    int pm1 = 0, pm25 = 0, pm10 = 0;
    if (SensorDriver::readPMS7003(pm1, pm25, pm10) == STATUS_OK) {
      s_last_pms_valid = true;
      s_last_pms_pm1 = pm1;
      s_last_pms_pm25 = pm25;
      s_last_pms_pm10 = pm10;
      return STATUS_OK;
    }
    return STATUS_ERROR;
  }

}
