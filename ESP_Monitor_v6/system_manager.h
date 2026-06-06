#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include "types.h"
#include "logger.h"
#include "sensor_service.h"
#include "time_service.h"
#include "web_service.h"
#include "data_service.h"

// ============ SYSTEM MANAGER ============
// Coordinates all services:
// - Initializes hardware/drivers/services
// - Manages main loop: sensor reading, logging, web server
// - Handles error recovery and watchdog
// - Manages OLED display cycling

namespace SystemManager {

  // Initialize entire system (call once in setup)
  Status_t init();
  
  // Main update loop (call repeatedly in loop)
  // Returns number of milliseconds to wait before next call
  uint32_t update();
  
  // Get current system state
  const SystemState_t& getSystemState();
  
  // Get current sensor data
  const SensorData_t& getLatestSensorData();
  
  // Manual time set from web
  Status_t setTimeFromWeb(uint32_t unix_timestamp);
  
  // Trigger NTP sync (can be called manually)
  Status_t triggerNTPSync();
  
  // Get system uptime in milliseconds
  uint32_t getUptimeMS();
  
  // Reset watchdog timer
  void resetWatchdog();
  
  // Log system stats (for debugging)
  void logSystemStats();

}

#endif // SYSTEM_MANAGER_H
