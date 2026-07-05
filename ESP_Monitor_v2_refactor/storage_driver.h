#ifndef STORAGE_DRIVER_H
#define STORAGE_DRIVER_H

#include <Arduino.h>
#include <SD.h>
#include "config.h"
#include "types.h"

// ============ SD CARD DRIVER ============
// Low-level SD operations: open, write, close
// Service layer handles CSV formatting and error recovery

namespace StorageDriver {

  // Initialize SD card
  Status_t init();
  
  // Check if SD card is available
  uint8_t isReady();
  
  // Get SD card stats
  void getStats(float& totalMB, float& usedMB, float& freeMB);
  
  // Open CSV file for append
  File openCSVFile(const char* filename);
  
  // Write raw bytes to file
  uint16_t writeFile(File& file, const uint8_t* data, uint16_t len);
  
  // Write formatted string to file
  uint16_t writeFileLine(File& file, const char* line);
  
  // Close file
  void closeFile(File& file);
  
  // Check if file exists
  uint8_t fileExists(const char* filename);
  
  // Delete file
  Status_t deleteFile(const char* filename);
  
  // Read entire file into buffer (max 8KB to avoid memory issues)
  // Returns bytes read, or -1 on error
  int16_t readFile(const char* filename, char* buffer, uint16_t max_size);

}

#endif // STORAGE_DRIVER_H
