#ifndef WEB_PAGES_H
#define WEB_PAGES_H

#include <Arduino.h>

// Hàm helper sinh trang dashboard chính
// HTML/CSS/JS giữ nguyên từ bản v5
String getDashboardPage(float totalMB, float usedMB, float freeMB, float percentUsed);

#endif
