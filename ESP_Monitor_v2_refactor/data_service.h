#ifndef DATA_SERVICE_H
#define DATA_SERVICE_H

#include <Arduino.h>
#include "config.h"
#include "types.h"

// ============ DATA SERVICE ============
// CSV parsing, JSON formatting, data transformation
// Uses char buffers instead of String to avoid heap fragmentation

namespace DataService {

  // Format SensorData_t as CSV line (semicolon-separated)
  // Returns length of formatted string, or -1 on error
  int16_t formatCSVLine(const SensorData_t& data, char* buffer, size_t max_len);
  
  // Format SensorData_t as JSON (live data)
  // Returns length of formatted string, or -1 on error
  int16_t formatLiveJSON(const SensorData_t& data, char* buffer, size_t max_len);
  
  // Parse CSV file and extract unique dates
  // Returns array of date strings (internal buffer)
  // Call immediately before using result - not thread-safe
  uint16_t extractDatesFromCSV(const char* csv_data, char dates[][11], uint16_t max_dates);
  
  // Parse CSV and extract data for specific date
  // Returns date_count for that day
  uint16_t extractDayDataFromCSV(
    const char* csv_data,
    const char* target_date,
    char times[][9],
    float pm25_vals[],
    float temps[],
    float humid[],
    float press[],
    uint16_t max_samples
  );
  
  // Calculate interval for downsampling (144 points max)
  uint16_t calculateDownsampleInterval(uint16_t total_points);
  
  // Format downsampled data as JSON
  int16_t formatHistoryJSON(
    const char times[][9],
    const float pm25[],
    const float temps[],
    const float humid[],
    const float press[],
    uint16_t count,
    char* buffer,
    size_t max_len
  );

}

#endif // DATA_SERVICE_H
