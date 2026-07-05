#ifndef DATA_SERVICE_H
#define DATA_SERVICE_H

#include <Arduino.h>
#include "config.h"
#include "types.h"

// ============ DATA SERVICE ============
// Parse CSV và format JSON
// Dùng buffer char để tránh碎 bộ nhớ

namespace DataService {

  // Format dữ liệu thành dòng CSV
  // Trả về độ dài chuỗi, hoặc -1 nếu lỗi
  int16_t formatCSVLine(const SensorData_t& data, char* buffer, size_t max_len);
  
  // Format dữ liệu thành JSON cho live data
  // Trả về độ dài chuỗi, hoặc -1 nếu lỗi
  int16_t formatLiveJSON(const SensorData_t& data, char* buffer, size_t max_len);
  
  // Parse file CSV và lấy các ngày duy nhất
  // Trả về mảng ngày trong buffer nội bộ
  // Dùng ngay sau khi gọi, không thread-safe
  uint16_t extractDatesFromCSV(const char* csv_data, char dates[][11], uint16_t max_dates);
  
  // Parse CSV và lấy dữ liệu cho một ngày cụ thể
  // Trả về số mẫu của ngày đó
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
  
  // Tính khoảng cách downsample (tối đa 144 điểm)
  uint16_t calculateDownsampleInterval(uint16_t total_points);
  
  // Format dữ liệu downsample thành JSON
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
