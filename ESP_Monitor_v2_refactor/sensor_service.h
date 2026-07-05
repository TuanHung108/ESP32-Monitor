#ifndef SENSOR_SERVICE_H
#define SENSOR_SERVICE_H

#include <Arduino.h>
#include "config.h"
#include "types.h"
#include "logger.h"
#include "sensor_driver.h"

// ============ SENSOR SERVICE ============
// Service đọc cảm biến với:
// - Validation (check ranges)
// - Error recovery (reinit on failure)
// - Fallback values (use last-known-good data)

namespace SensorService {

  // Initialize all sensors
  Status_t init();
  
  // Poll PMS7003 frequently to keep UART buffer drained
  Status_t pollPMS7003();
  
  // Read all sensors once, return consolidated SensorData_t
  // Performs validation and error recovery
  // Tries up to 3 times if sensor fails, with reinit between retries
  Status_t readAllSensors(SensorData_t& data);
  
  // Individual read with recovery (internal use)
  Status_t _readBME280WithRecovery(float& temp, float& humidity, float& pressure, uint32_t now_ms);
  Status_t _readBMP280WithRecovery(float& temp, float& pressure, uint32_t now_ms);
  Status_t _readHTU21DWithRecovery(float& temp, float& humidity, uint32_t now_ms);
  Status_t _readSHT31WithRecovery(float& temp, float& humidity, uint32_t now_ms);
  Status_t _readAHT20WithRecovery(float& temp, float& humidity, uint32_t now_ms);
  Status_t _readPMS7003WithRecovery(int& pm1, int& pm25, int& pm10);
  Status_t _readSDS011WithRecovery(float& pm25, float& pm10);

}

#endif // SENSOR_SERVICE_H
