#ifndef STORAGE_DRIVER_H
#define STORAGE_DRIVER_H

#include <Arduino.h>
#include <SD.h>
#include "config.h"
#include "types.h"

// ============ DRIVER SD ============
// Mở, ghi, đóng file
// Service bên trên lo phần format CSV và xử lý lỗi

namespace StorageDriver {

  // Khởi tạo thẻ SD
  Status_t init();
  
  // Kiểm tra thẻ SD còn dùng được không
  uint8_t isReady();
  
  // Lấy thông tin thẻ SD
  void getStats(float& totalMB, float& usedMB, float& freeMB);
  
  // Mở file CSV để ghi tiếp
  File openCSVFile(const char* filename);
  
  // Ghi bytes thô vào file
  uint16_t writeFile(File& file, const uint8_t* data, uint16_t len);
  
  // Ghi chuỗi đã format vào file
  uint16_t writeFileLine(File& file, const char* line);
  
  // Đóng file
  void closeFile(File& file);
  
  // Kiểm tra file có tồn tại không
  uint8_t fileExists(const char* filename);
  
  // Xóa file
  Status_t deleteFile(const char* filename);
  
  // Đọc toàn bộ file vào buffer (tối đa 8KB)
  // Trả về số byte đọc được, hoặc -1 nếu lỗi
  int16_t readFile(const char* filename, char* buffer, uint16_t max_size);

}

#endif // STORAGE_DRIVER_H
