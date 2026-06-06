#ifndef TIME_SERVICE_H
#define TIME_SERVICE_H

#include <Arduino.h>
#include <time.h>
#include "config.h"
#include "types.h"
#include "logger.h"

// ============ TIME SERVICE ============
// NTP synchronization and RTC management

namespace TimeService {

  // Initialize time system
  Status_t init();
  
  // Synchronize time with NTP server
  // Blocks until NTP response or timeout
  Status_t syncWithNTP();
  
  // Check if NTP has been synced at least once this session
  uint8_t isNTPSynced();
  
  // Update internal time from RTC
  Status_t updateFromRTC();
  
  // Get current time
  Status_t getCurrentTime(char* date_str, char* time_str, uint32_t& unix_time);
  
  // Manual time set (from web request)
  Status_t setTimeFromWeb(uint32_t unix_timestamp);
  
  // Check if it's time to sync NTP again
  uint8_t shouldSyncNTP();
  
  // Format unix timestamp as date "YYYY/MM/DD"
  void formatUnixDate(uint32_t unix_time, char* buffer);
  
  // Format unix timestamp as time "HH:MM:SS"
  void formatUnixTime(uint32_t unix_time, char* buffer);

}

#endif // TIME_SERVICE_H
