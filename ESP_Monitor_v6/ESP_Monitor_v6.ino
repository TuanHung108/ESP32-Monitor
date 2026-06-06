/*
 * ESP32 Environmental Monitoring System v6
 * 
 * ARCHITECTURE:
 * - Application Layer (main): dispatch only
 * - Service Layer: business logic
 * - Driver Layer: hardware I/O
 * 
 * Improvements over v5:
 * - Structured logging (INFO/WARN/ERROR)
 * - Error recovery with watchdog timer
 * - Reduced global variables via struct consolidation
 * - Layer-based architecture for maintainability
 * - Char buffer instead of String in hot paths
 * 
 * Hardware:
 * - PMS7003 (Serial2: RX=16, TX=17)
 * - SDS011 (Serial1: RX=26, TX=27)
 * - BME280 (I2C: 0x76)
 * - BMP280 (I2C: 0x77)
 * - SHT31 (I2C: 0x44)
 * - HTU21D (I2C)
 * - AHT20 (I2C)
 * - DS3231 (I2C)
 * - SSD1306 OLED (I2C: 0x3C)
 * - SD card (SPI: CS=5)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// Configuration & types
#include "config.h"
#include "types.h"
#include "logger.h"

// Layers
#include "system_manager.h"

// ============ SETUP ============
void setup() {
  SystemManager::init();
}

// ============ MAIN LOOP ============
void loop() {
  uint32_t wait_ms = SystemManager::update();
  delay(wait_ms);
}
