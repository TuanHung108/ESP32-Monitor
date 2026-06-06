#ifndef WEB_PAGES_H
#define WEB_PAGES_H

#include <Arduino.h>

// Hàm helper để sinh trang giao diện chính Dashboard
// (HTML/CSS/JS unchanged from v5 - copied as-is)
String getDashboardPage(float totalMB, float usedMB, float freeMB, float percentUsed);

#endif
