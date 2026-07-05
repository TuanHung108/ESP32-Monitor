// Implementation of storage_driver.h
// SD card operations

#include "storage_driver.h"
#include <SPI.h>

namespace StorageDriver {

  Status_t init() {
    // Initialize SPI with correct pins for ESP32
    // MOSI=GPIO23, MISO=GPIO19, CLK=GPIO18, CS=GPIO5 (default)
    SPI.begin(18, 19, 23, SD_CS_PIN);
    uint32_t sd_ready_ms = millis();
    while (millis() - sd_ready_ms < 100) {
      yield();
    }
    
    if (!SD.begin(SD_CS_PIN)) {
      return STATUS_ERROR;
    }
    return STATUS_OK;
  }
  
  uint8_t isReady() {
    return SD.exists("/") ? 1 : 0;
  }
  
  void getStats(float& totalMB, float& usedMB, float& freeMB) {
    if (!isReady()) {
      totalMB = 0.0f;
      usedMB = 0.0f;
      freeMB = 0.0f;
      return;
    }
    totalMB = SD.totalBytes() / (1024.0 * 1024.0);
    usedMB = SD.usedBytes() / (1024.0 * 1024.0);
    freeMB = totalMB - usedMB;
  }
  
  File openCSVFile(const char* filename) {
    return SD.open(filename, FILE_APPEND);
  }
  
  uint16_t writeFile(File& file, const uint8_t* data, uint16_t len) {
    if (!file) return 0;
    return file.write(data, len);
  }
  
  uint16_t writeFileLine(File& file, const char* line) {
    if (!file) return 0;
    return file.println(line);
  }
  
  void closeFile(File& file) {
    if (file) file.close();
  }
  
  uint8_t fileExists(const char* filename) {
    return SD.exists(filename) ? 1 : 0;
  }
  
  Status_t deleteFile(const char* filename) {
    if (!SD.exists(filename)) {
      return STATUS_ERROR;
    }
    if (SD.remove(filename)) {
      return STATUS_OK;
    }
    return STATUS_ERROR;
  }
  
  int16_t readFile(const char* filename, char* buffer, uint16_t max_size) {
    File file = SD.open(filename, FILE_READ);
    if (!file) {
      return -1;
    }
    
    int16_t bytes_read = 0;
    while (file.available() && bytes_read < (int16_t)(max_size - 1)) {
      buffer[bytes_read++] = file.read();
    }
    buffer[bytes_read] = '\0';
    
    file.close();
    return bytes_read;
  }

}
