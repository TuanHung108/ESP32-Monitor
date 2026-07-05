// Implement đồng bộ thời gian và helper thời gian
// NTP synchronization and RTC management

#include "time_service.h"
#include <RTClib.h>
#include <time.h>
#include <sys/time.h>

// Time service state
static uint8_t s_ntp_synced = 0;
static uint32_t s_last_ntp_sync = 0;
static uint32_t s_system_time = 0;
static RTC_DS3231 g_rtc;

static uint32_t _dateTimeToUnix(const DateTime& dt);
static void _formatDate(const DateTime& dt, char* buffer, size_t len);
static void _formatTime(const DateTime& dt, char* buffer, size_t len);

namespace TimeService {

  Status_t init() {
    if (!g_rtc.begin()) {
      g_logger.error("TimeSvc", "RTC init failed");
      return STATUS_ERROR;
    }

    // Read RTC and set system time
    DateTime now = g_rtc.now();
    s_system_time = _dateTimeToUnix(now);

    if (g_rtc.lostPower()) {
      g_logger.warn("TimeSvc", "RTC battery low");
    }

    g_logger.info("TimeSvc", "Initialized");
    return STATUS_OK;
  }
  
  Status_t syncWithNTP() {
    g_logger.info("TimeSvc", "NTP sync starting...");
    
    // Configure timezone and NTP
    configTime(TIMEZONE_OFFSET * 3600, 0, NTP_SERVER);
    
    // Wait for NTP response (up to 10 seconds)
    time_t now = time(nullptr);
    uint32_t ntp_start_ms = millis();
    while (now < 1000000000 && millis() - ntp_start_ms < 10000) {
      if (millis() - ntp_start_ms >= 200) {
        now = time(nullptr);
      }
      yield();
    }
    
    if (now < 1000000000) {
      g_logger.warn("TimeSvc", "NTP timeout");
      return STATUS_TIMEOUT;
    }
    
    // NTP successful - update RTC
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    DateTime dt(
      timeinfo.tm_year + 1900,
      timeinfo.tm_mon + 1,
      timeinfo.tm_mday,
      timeinfo.tm_hour,
      timeinfo.tm_min,
      timeinfo.tm_sec
    );
    g_rtc.adjust(dt);
    
    s_ntp_synced = 1;
    s_last_ntp_sync = millis();
    s_system_time = (uint32_t)now;
    
    g_logger.info("TimeSvc", "NTP synced OK");
    return STATUS_OK;
  }
  
  uint8_t isNTPSynced() {
    return s_ntp_synced;
  }
  
  Status_t updateFromRTC() {
    DateTime now = g_rtc.now();
    s_system_time = _dateTimeToUnix(now);
    return STATUS_OK;
  }
  
  Status_t getCurrentTime(char* date_str, char* time_str, uint32_t& unix_time) {
    if (!date_str || !time_str) {
      return STATUS_ERROR;
    }

    DateTime now = g_rtc.now();
    _formatDate(now, date_str, 11);
    _formatTime(now, time_str, 9);
    unix_time = _dateTimeToUnix(now);

    return STATUS_OK;
  }
  
  Status_t setTimeFromWeb(uint32_t unix_timestamp) {
    g_logger.info("TimeSvc", "Time set from web");

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

    s_system_time = unix_timestamp;
    return STATUS_OK;
  }
  
  uint8_t shouldSyncNTP() {
    // First sync after 5s boot delay
    if (!s_ntp_synced && millis() > NTP_WARMUP_DELAY) {
      return 1;
    }
    
    // Periodic sync
    if (s_ntp_synced && millis() - s_last_ntp_sync > NTP_SYNC_INTERVAL) {
      return 1;
    }
    
    return 0;
  }
  
  void formatUnixDate(uint32_t unix_time, char* buffer) {
    time_t t = unix_time;
    struct tm* tm_info = localtime(&t);
    snprintf(buffer, 11, "%04d/%02d/%02d",
      tm_info->tm_year + 1900,
      tm_info->tm_mon + 1,
      tm_info->tm_mday
    );
  }
  
  void formatUnixTime(uint32_t unix_time, char* buffer) {
    time_t t = unix_time;
    struct tm* tm_info = localtime(&t);
    snprintf(buffer, 9, "%02d:%02d:%02d",
      tm_info->tm_hour,
      tm_info->tm_min,
      tm_info->tm_sec
    );
  }

}

static uint32_t _dateTimeToUnix(const DateTime& dt) {
  struct tm tm_info = {0};
  tm_info.tm_year = dt.year() - 1900;
  tm_info.tm_mon = dt.month() - 1;
  tm_info.tm_mday = dt.day();
  tm_info.tm_hour = dt.hour();
  tm_info.tm_min = dt.minute();
  tm_info.tm_sec = dt.second();
  tm_info.tm_isdst = -1;

  time_t t = mktime(&tm_info);
  return (uint32_t)t;
}

static void _formatDate(const DateTime& dt, char* buffer, size_t len) {
  snprintf(buffer, len, "%04d/%02d/%02d",
    dt.year(), dt.month(), dt.day());
}

static void _formatTime(const DateTime& dt, char* buffer, size_t len) {
  snprintf(buffer, len, "%02d:%02d:%02d",
    dt.hour(), dt.minute(), dt.second());
}
