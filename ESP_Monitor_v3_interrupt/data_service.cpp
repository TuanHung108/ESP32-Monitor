// Implement xử lý dữ liệu cho hệ thống
// CSV parsing and JSON formatting using char buffers

#include "data_service.h"
#include <string.h>
#include <stdio.h>

namespace DataService {

  int16_t formatCSVLine(const SensorData_t& data, char* buffer, size_t max_len) {
    int written = snprintf(buffer, max_len,
      "%s;%s;%d;%d;%d;%.1f;%.1f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f",
      data.date_str, data.time_str,
      data.pm1_0, data.pm2_5, data.pm10_0,
      data.sds_pm25, data.sds_pm10,
      data.bme_temp, data.bme_humi, data.bme_pres,
      data.bmp_temp, data.bmp_pres,
      data.htu_temp, data.htu_humi,
      data.sht_temp, data.sht_humi,
      data.aht_temp, data.aht_humi
    );
    
    return (written > 0 && written < (int)max_len) ? written : -1;
  }
  
  int16_t formatLiveJSON(const SensorData_t& data, char* buffer, size_t max_len) {
    int written = snprintf(buffer, max_len,
      "{"
      "\"sys_date\":\"%s\","
      "\"sys_time\":\"%s\","
      "\"pm10\":%d,"
      "\"pm25\":%d,"
      "\"pm100\":%d,"
      "\"sds_pm25\":%.2f,"
      "\"sds_pm10\":%.2f,"
      "\"bme_t\":%.2f,"
      "\"bme_h\":%.2f,"
      "\"bme_p\":%.2f,"
      "\"bmp_t\":%.2f,"
      "\"bmp_p\":%.2f,"
      "\"sht_t\":%.2f,"
      "\"sht_h\":%.2f,"
      "\"htu_t\":%.2f,"
      "\"htu_h\":%.2f,"
      "\"aht_t\":%.2f,"
      "\"aht_h\":%.2f,"
      "\"sd_ok\":%d,"
      "\"sd_total\":%.2f,"
      "\"sd_used\":%.2f,"
      "\"sd_free\":%.2f,"
      "\"sd_percent\":%.2f"
      "}",
      data.date_str, data.time_str,
      data.pm1_0, data.pm2_5, data.pm10_0,
      data.sds_pm25, data.sds_pm10,
      data.bme_temp, data.bme_humi, data.bme_pres,
      data.bmp_temp, data.bmp_pres,
      data.sht_temp, data.sht_humi,
      data.htu_temp, data.htu_humi,
      data.aht_temp, data.aht_humi,
      data.sd_ok,
      data.sd_total_mb,
      data.sd_used_mb,
      data.sd_free_mb,
      data.sd_percent
    );
    /*uint16_t count = 0;
    const char* line_start = csv_data;
    char prev_date[11] = {0};
    
    // Skip header line
    const char* first_newline = strchr(line_start, '\n');
    if (first_newline) {
      line_start = first_newline + 1;
    }
    
    while (count < max_dates && line_start) {
      // Find date field (first semicolon)
      const char* date_end = strchr(line_start, ';');
      if (!date_end) break;
      
      uint16_t date_len = date_end - line_start;
      if (date_len > 10) date_len = 10;
      
      char curr_date[11];
      strncpy(curr_date, line_start, date_len);
      curr_date[date_len] = '\0';
      
      // Check if this date is different from previous
      if (strcmp(curr_date, prev_date) != 0) {
        strcpy(dates[count], curr_date);
        strcpy(prev_date, curr_date);
        count++;
      }
      
      // Move to next line
      line_start = strchr(line_start, '\n');
      if (line_start) {
        line_start++;
      }
    }
    
    return count;*/
    return (written > 0 && written < (int)max_len) ? written : -1;
  }
  
  uint16_t extractDayDataFromCSV(
    const char* csv_data,
    const char* target_date,
    char times[][9],
    float pm25_vals[],
    float temps[],
    float humid[],
    float press[],
    uint16_t max_samples) {
    
    if (!csv_data || !target_date) return 0;
    
    uint16_t count = 0;
    const char* line_start = csv_data;
    
    // Skip header
    const char* first_newline = strchr(line_start, '\n');
    if (first_newline) {
      line_start = first_newline + 1;
    }
    
    while (count < max_samples && line_start) {
      // Extract date (first field)
      const char* semi1 = strchr(line_start, ';');
      if (!semi1) break;
      
      uint16_t date_len = semi1 - line_start;
      if (date_len > 10) date_len = 10;
      
      char date_str[11];
      strncpy(date_str, line_start, date_len);
      date_str[date_len] = '\0';
      
      // Check if matches target date
      if (strcmp(date_str, target_date) == 0) {
        // Extract time and values
        const char* semi2 = strchr(semi1 + 1, ';');
        const char* semi3 = strchr(semi2 + 1, ';');
        const char* semi4 = strchr(semi3 + 1, ';');
        const char* semi5 = strchr(semi4 + 1, ';');
        const char* semi6 = strchr(semi5 + 1, ';');
        const char* semi7 = strchr(semi6 + 1, ';');
        const char* semi8 = strchr(semi7 + 1, ';');
        
        if (semi2 && semi8) {
          // Time
          strncpy(times[count], semi1 + 1, 8);
          times[count][8] = '\0';
          
          // PM2.5 (PMS field)
          const char* semi4 = strchr(semi3 + 1, ';');
          if (!semi4) {
            line_start = strchr(line_start, '\n');
            if (line_start) line_start++;
            continue;
          }
          pm25_vals[count] = atof(semi3 + 1);
          
          // Temperature (field 8 from start: date, time, pm1, pm25, pm10, sds25, sds10, bme_temp, ...)
          temps[count] = atof(semi7 + 1);
          
          // Humidity (next field)
          const char* semi9 = strchr(semi8 + 1, ';');
          if (semi9) {
            humid[count] = atof(semi8 + 1);
            // Pressure (next field)
            const char* semi10 = strchr(semi9 + 1, ';');
            if (semi10) {
              press[count] = atof(semi9 + 1);
            }
          }
          
          count++;
        }
      }
      
      // Move to next line
      line_start = strchr(line_start, '\n');
      if (line_start) {
        line_start++;
      }
    }
    
    return count;
  }
  
  uint16_t calculateDownsampleInterval(uint16_t total_points) {
    if (total_points <= 144) return 1;
    return (total_points + 143) / 144;
  }
  
  int16_t formatHistoryJSON(
    const char times[][9],
    const float pm25[],
    const float temps[],
    const float humid[],
    const float press[],
    uint16_t count,
    char* buffer,
    size_t max_len) {
    
    if (count == 0) {
      return snprintf(buffer, max_len, 
        "{\"timestamps\":[],\"pms25\":[],\"temp\":[],\"hum\":[],\"press\":[]}");
    }
    
    int pos = 0;
    
    // Start JSON
    pos += snprintf(buffer + pos, max_len - pos, "{\"timestamps\":[");
    
    // Times
    for (uint16_t i = 0; i < count; i++) {
      if (i > 0) pos += snprintf(buffer + pos, max_len - pos, ",");
      pos += snprintf(buffer + pos, max_len - pos, "\"%s\"", times[i]);
    }
    
    pos += snprintf(buffer + pos, max_len - pos, "],\"pms25\":[");
    
    // PM2.5 values
    for (uint16_t i = 0; i < count; i++) {
      if (i > 0) pos += snprintf(buffer + pos, max_len - pos, ",");
      pos += snprintf(buffer + pos, max_len - pos, "%.2f", pm25[i]);
    }
    
    pos += snprintf(buffer + pos, max_len - pos, "],\"temp\":[");
    
    // Temperature values
    for (uint16_t i = 0; i < count; i++) {
      if (i > 0) pos += snprintf(buffer + pos, max_len - pos, ",");
      pos += snprintf(buffer + pos, max_len - pos, "%.2f", temps[i]);
    }
    
    pos += snprintf(buffer + pos, max_len - pos, "],\"hum\":[");
    
    // Humidity values
    for (uint16_t i = 0; i < count; i++) {
      if (i > 0) pos += snprintf(buffer + pos, max_len - pos, ",");
      pos += snprintf(buffer + pos, max_len - pos, "%.2f", humid[i]);
    }
    
    pos += snprintf(buffer + pos, max_len - pos, "],\"press\":[");
    
    // Pressure values
    for (uint16_t i = 0; i < count; i++) {
      if (i > 0) pos += snprintf(buffer + pos, max_len - pos, ",");
      pos += snprintf(buffer + pos, max_len - pos, "%.2f", press[i]);
    }
    
    pos += snprintf(buffer + pos, max_len - pos, "]}");
    
    return pos;
  }

}
