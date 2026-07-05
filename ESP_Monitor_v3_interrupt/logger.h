#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include "config.h"
#include "types.h"

// ============ LOGGER CÓ CẤU TRÚC ============
// Thay Serial.println() bằng log có phân loại
// INFO: Hoạt động bình thường
// WARN: Vấn đề có thể khôi phục
// ERROR: Lỗi nghiêm trọng

class Logger {
private:
  static const uint16_t BUFFER_SIZE = LOG_BUFFER_SIZE;
  LogEntry_t logs[BUFFER_SIZE];
  uint16_t write_idx = 0;
  
public:
  // Initialize logger (call once in setup)
  void init() {
    for (uint16_t i = 0; i < BUFFER_SIZE; i++) {
      logs[i].timestamp = 0;
      logs[i].level = LOG_LEVEL_INFO;
      logs[i].tag[0] = '\0';
      logs[i].message[0] = '\0';
    }
  }
  
  // Log info message (normal operation)
  void info(const char* tag, const char* msg) {
    _log(LOG_LEVEL_INFO, tag, msg, "INFO");
  }
  
  // Log warning (recoverable issue)
  void warn(const char* tag, const char* msg) {
    _log(LOG_LEVEL_WARN, tag, msg, "WARN");
  }
  
  // Log error (critical issue)
  void error(const char* tag, const char* msg) {
    _log(LOG_LEVEL_ERROR, tag, msg, "ERROR");
  }
  
  // Print all logs (for debugging)
  void printAll() {
    Serial.println("\n=== SYSTEM LOGS ===");
    for (uint16_t i = 0; i < BUFFER_SIZE; i++) {
      if (logs[i].timestamp == 0) continue;
      Serial.printf("[%u] %s | %s: %s\n", 
        logs[i].timestamp,
        _levelToStr(logs[i].level),
        logs[i].tag,
        logs[i].message);
    }
    Serial.println("===================\n");
  }

private:
  void _log(uint8_t level, const char* tag, const char* msg, const char* level_str) {
    // Store in circular buffer
    logs[write_idx].timestamp = millis() / 1000;  // Convert to seconds
    logs[write_idx].level = level;
    strncpy(logs[write_idx].tag, tag, sizeof(logs[write_idx].tag) - 1);
    strncpy(logs[write_idx].message, msg, sizeof(logs[write_idx].message) - 1);
    logs[write_idx].tag[sizeof(logs[write_idx].tag) - 1] = '\0';
    logs[write_idx].message[sizeof(logs[write_idx].message) - 1] = '\0';
    
    write_idx = (write_idx + 1) % BUFFER_SIZE;
    
    // Also print to Serial for real-time monitoring
    if (ENABLE_DEBUG) {
      uint32_t uptime_sec = millis() / 1000;
      Serial.printf("[%05u] %-5s | %s: %s\n", uptime_sec, level_str, tag, msg);
    }
  }
  
  const char* _levelToStr(uint8_t level) {
    switch (level) {
      case LOG_LEVEL_INFO:  return "INFO";
      case LOG_LEVEL_WARN:  return "WARN";
      case LOG_LEVEL_ERROR: return "ERROR";
      default:              return "?";
    }
  }
};

// Global logger instance
extern Logger g_logger;

#endif // LOGGER_H
