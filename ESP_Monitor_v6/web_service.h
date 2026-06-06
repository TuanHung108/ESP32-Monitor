#ifndef WEB_SERVICE_H
#define WEB_SERVICE_H

#include <Arduino.h>
#include <WebServer.h>
#include "config.h"
#include "types.h"
#include "data_service.h"

// ============ WEB SERVICE ============
// HTTP request handlers
// Routes defined in main file, implementations here

extern WebServer g_server;

namespace WebService {

  // Initialize web service
  Status_t init();
  
  // Start web server
  void start();
  
  // Handle client connections (call in loop)
  void handleClient();
  
  // ---- HTTP Handlers ----
  // These are called by WebServer when routes match
  
  void handleRoot();              // GET /
  void handleLiveJSON();          // GET /live-data
  void handleHistoryJSON();       // GET /history-data?date=YYYY-MM-DD
  void handleDaysJSON();          // GET /days
  void handleDownloadCSV();       // GET /download
  void handleDeleteData();        // GET /delete?pass=XXXX
  void handleSetTime();           // GET /settime?t=UNIX_TIMESTAMP
  
  // Update live data for broadcast (call from main sensor reading)
  void setCurrentSensorData(const SensorData_t& data);
  
  // Get current sensor data (for JSON responses)
  const SensorData_t& getCurrentSensorData();

}

#endif // WEB_SERVICE_H
