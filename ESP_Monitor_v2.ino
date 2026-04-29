#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include <Adafruit_HTU21DF.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_SHT31.h>
#include <PMS.h>
#include <time.h>

// ---------------- KHAI BÁO HÀM THUỘC TÍNH ----------------
void handleRoot();
void handleDownload();
void handleDelete();
void readSensors();
void logToSDCard();
void updateOLED();
void syncTimeWithNTP();

// ---------------- CẤU HÌNH PHẦN CỨNG ----------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
#define SD_CS_PIN 5 // Chân CS thẻ SD

// ---------------- CẤU HÌNH WIFI & WEB ----------------
const char* ssid = "BinhHung";
const char* password = "7474747474";
WebServer server(80);

// ---------------- KHỞI TẠO CẢM BIẾN ----------------
RTC_DS3231 rtc;
Adafruit_HTU21DF htu = Adafruit_HTU21DF();
Adafruit_BME280 bme; 
Adafruit_BMP280 bmp;
Adafruit_AHTX0 aht;
Adafruit_SHT31 sht = Adafruit_SHT31();
PMS pms(Serial2);
PMS::DATA data;

// ---------------- BIẾN TOÀN CỤC CHỨA DỮ LIỆU --------
int pm1 = 0, pm25 = 0, pm10 = 0;
float t_bme=0, h_bme=0, p_bme=0;
float t_bmp=0, p_bmp=0;
float t_htu=0, h_htu=0;
float t_sht=0, h_sht=0;
float t_aht=0, h_aht=0;
String currentTime = "";
String currentDate = "";

unsigned long previousMillis = 0;
const long interval = 5000; // 5 giây lấy mẫu 1 lần

// ---------------- NTP TIMER ----------------
unsigned long lastNtpSync = 0;
const unsigned long ntpInterval = 21600000; // 6 giờ
bool ntpDoneOnce = false;

int oledPage = 0; // Biến cuộn trang OLED

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  Serial2.begin(9600, SERIAL_8N1, 16, 17); // PMS7003

  // 1. Khởi tạo OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Loi OLED");
  }
  display.clearDisplay(); display.setTextSize(1); display.setTextColor(WHITE);
  display.setCursor(0,0); display.println("Dang khoi tao..."); display.display();

  // 2. Khởi tạo SD Card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Loi the SD Card!");
  } else {
    Serial.println("The SD san sang.");
  }

  // 3. Khởi tạo WiFi
  IPAddress local_IP(192,168,1,200);
  IPAddress gateway(192,168,1,1);
  IPAddress subnet(255,255,255,0);
  IPAddress dns(8,8,8,8);

  WiFi.config(local_IP, gateway, subnet, dns);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi OK! IP: " + WiFi.localIP().toString());

  
  // 4. Khởi tạo Web Server (Đã bao gồm link Download và Delete)
  server.on("/", handleRoot);
  server.on("/download", handleDownload);
  server.on("/delete", handleDelete); // THÊM DÒNG NÀY ĐỂ KÍCH HOẠT XÓA
  server.begin();

  // 5. Khởi tạo Cảm biến
  if (!rtc.begin()) Serial.println("Loi RTC");
  if (rtc.lostPower()) {
    Serial.println("RTC mat nguon, dang set lai thoi gian!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  if (!htu.begin()) Serial.println("Loi HTU21D");
  if (!bme.begin(0x76)) Serial.println("Loi BME280(0x76)");
  if (!bmp.begin(0x77)) Serial.println("Loi BMP280(0x77)"); // Chân SDO nối 3.3V
  if (!aht.begin()) Serial.println("Loi AHT20");
  if (!sht.begin()) Serial.println("Loi SHT31");
}

// ================= LOOP =================
void loop() {

  server.handleClient();

  // ================= NTP =================
  if (WiFi.status() == WL_CONNECTED) {

    // Sync lần đầu sau 5s
    if (!ntpDoneOnce && millis() > 5000) {
      Serial.println("NTP lan dau...");
      syncTimeWithNTP();
      ntpDoneOnce = true;
      lastNtpSync = millis();
    }

  // Sync mỗi 6 giờ (tránh trùng lúc log)
    if (millis() - lastNtpSync > ntpInterval) {
      if (millis() - previousMillis > 1000) { // tránh trùng lúc đang log
        Serial.println("NTP dinh ky...");
        syncTimeWithNTP();
        lastNtpSync = millis();
      }
    }
  }

  // PMS
  if (pms.read(data)) {
    pm1 = data.PM_AE_UG_1_0;
    pm25 = data.PM_AE_UG_2_5;
    pm10 = data.PM_AE_UG_10_0;
  }

  // Lấy mẫu
  if (millis() - previousMillis >= interval) {
    previousMillis = millis();

    readSensors();
    logToSDCard();
    updateOLED();

    Serial.println("[" + currentTime + "] Logged");
  }
}
// ================= CÁC HÀM XỬ LÝ =================
// ================= NTP =================
void syncTimeWithNTP() {

  static bool ntpConfigured = false;

  // Chỉ config NTP 1 lần duy nhất
  if (!ntpConfigured) {
    configTime(7*3600, 0, "pool.ntp.org");
    ntpConfigured = true;
  }

  Serial.print("Dong bo NTP");

  time_t now = time(nullptr);
  int retry = 0;

  // Giảm delay + giảm thời gian block
  while (now < 1000000000 && retry < 10) {
    delay(200);  // trước là 500ms → giảm xuống
    Serial.print(".");
    now = time(nullptr);
    retry++;
  }

  if (now < 1000000000) {
    Serial.println("\nLoi NTP");
    return;
  }

  Serial.println("\nOK NTP");

  struct tm timeinfo;
  if (!localtime_r(&now, &timeinfo)) {
    Serial.println("Loi localtime");
    return;
  }

  rtc.adjust(DateTime(
    timeinfo.tm_year + 1900,
    timeinfo.tm_mon + 1,
    timeinfo.tm_mday,
    timeinfo.tm_hour,
    timeinfo.tm_min,
    timeinfo.tm_sec
  ));

  Serial.println("RTC updated");
}

void readSensors() {
  DateTime now = rtc.now();
  
  char dateBuf[20];
  sprintf(dateBuf, "%04d/%02d/%02d", now.year(), now.month(), now.day());
  currentDate = String(dateBuf);
  
  char timeBuf[20];
  sprintf(timeBuf, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  currentTime = String(timeBuf);

  t_bme = bme.readTemperature(); h_bme = bme.readHumidity(); p_bme = bme.readPressure() / 100.0F;
  t_bmp = bmp.readTemperature(); p_bmp = bmp.readPressure() / 100.0F;
  t_htu = htu.readTemperature(); h_htu = htu.readHumidity();
  t_sht = sht.readTemperature(); h_sht = sht.readHumidity();
  
  sensors_event_t h_event, t_event;
  aht.getEvent(&h_event, &t_event);
  t_aht = t_event.temperature; h_aht = h_event.relative_humidity;
}

void logToSDCard() {
  String fileName = "/datalog.csv";
  bool fileExists = SD.exists(fileName);
  File dataFile = SD.open(fileName, FILE_APPEND);
  
  if (dataFile) {
    if (!fileExists) {
      // Ép BOM UTF-8 để không lỗi font Tiếng Việt
      dataFile.write(0xEF); 
      dataFile.write(0xBB); 
      dataFile.write(0xBF);
      dataFile.println("Ngày đo;Giờ đo;PM1.0 (µg/m³);PM2.5 (µg/m³);PM10 (µg/m³);BME_Nhiệt (°C);BME_Ẩm (%);BME_Áp suất (hPa);BMP_Nhiệt (°C);BMP_Áp suất (hPa);HTU_Nhiệt (°C);HTU_Ẩm (%);SHT_Nhiệt (°C);SHT_Ẩm (%);AHT_Nhiệt (°C);AHT_Ẩm (%)");
    }
    
    dataFile.printf("%s;%s;%d;%d;%d;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f\n",
      currentDate.c_str(), currentTime.c_str(), pm1, pm25, pm10, 
      t_bme, h_bme, p_bme, t_bmp, p_bmp, t_htu, h_htu, t_sht, h_sht, t_aht, h_aht
    );
    dataFile.close();
  } else {
    Serial.println("Loi: Khong the mo the SD!");
  }
}

void updateOLED() {
  display.clearDisplay();
  display.setCursor(0, 0);
  
  display.println("Date: " + currentDate);
  display.println("Time: " + currentTime);
  display.drawLine(0, 18, 128, 18, WHITE);

  display.setCursor(0, 22);
  if (oledPage == 0) {
    display.println("[Page 1]");
    display.printf("PM2.5: %d ug/m3\n", pm25);
    display.printf("BME T:%.1fC, Ap:%.0f\n", t_bme, p_bme);
    display.printf("BME H:%.1f%%\n", h_bme);
  } 
  else if (oledPage == 1) {
    display.println("[Page 2]");
    display.printf("BMP T:%.1fC, Ap:%.0f\n", t_bmp, p_bmp);
    display.printf("HTU T:%.1fC, H:%.1f%%\n", t_htu, h_htu);
  }
  else if (oledPage == 2) {
    display.println("[Page 3]");
    display.printf("AHT T:%.1fC, H:%.1f%%\n", t_aht, h_aht);
    display.printf("SHT T:%.1fC, H:%.1f%%\n", t_sht, h_sht);
  }

  display.display();
  oledPage++;  if (oledPage > 2) oledPage = 0;
}

void handleRoot() {
  // 1. TÍNH TOÁN DUNG LƯỢNG THẺ NHỚ (đơn vị Megabyte)
  float totalMB = SD.totalBytes() / (1024.0 * 1024.0);
  float usedMB  = SD.usedBytes() / (1024.0 * 1024.0);
  float freeMB  = totalMB - usedMB;
  float percentUsed = (totalMB > 0) ? (usedMB / totalMB * 100.0) : 0;

  // 2. KHỞI TẠO GIAO DIỆN WEB
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta http-equiv='refresh' content='5'>"; 
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Bảng Dữ Liệu Quan Trắc</title>";
  
  html += "<style>";
  html += "body{font-family:'Segoe UI',Arial,sans-serif; background-color:#f4f7f6; color:#333; margin:0; padding:20px;}";
  html += "h1{text-align:center; color:#2c3e50;}";
  html += ".container{background:#fff; padding:20px; border-radius:10px; box-shadow:0 4px 10px rgba(0,0,0,0.1); max-width:1400px; margin:auto;}";
  html += ".table-wrapper{overflow-x:auto;}";
  html += "table{width:100%; border-collapse:collapse; white-space:nowrap; margin-top:10px;}";
  html += "th, td{border:1px solid #ddd; padding:12px; text-align:center;}";
  html += "th{background-color:#3498db; color:white; font-weight:bold;}";
  html += "tr:nth-child(even){background-color:#f9f9f9;}";
  html += "tr:hover{background-color:#f1f1f1;}";
  
  /* Nút Tải và Xóa */
  html += ".btn{display:table; margin:15px auto; padding:15px 30px; background-color:#27ae60; color:white; text-decoration:none; font-weight:bold; border-radius:50px; font-size:16px; transition:0.3s;}";
  html += ".btn:hover{background-color:#2ecc71;}";
  html += ".btn-danger{display:table; margin:15px auto; padding:10px 20px; background-color:#e74c3c; color:white; text-decoration:none; font-weight:bold; border-radius:5px; font-size:14px; transition:0.3s;}";
  html += ".btn-danger:hover{background-color:#c0392b;}";
  html += "</style></head><body>";
  
  html += "<div class='container'>";
  html += "<h1>BÁO CÁO DỮ LIỆU QUAN TRẮC</h1>";
  html += "<p style='text-align:center;font-size:15px;'>Tự động quét đo và tải lại Web sau mỗi 5 giây.</p>";
  
  // BẢNG BÁO CÁO THÔNG SỐ
  html += "<div class='table-wrapper'><table>";
  html += "<tr><th>Ngày đo</th><th>Giờ Đo</th><th>PM1.0<br>(µg/m³)</th><th>PM2.5<br>(µg/m³)</th><th>PM10<br>(µg/m³)</th>";
  html += "<th>BME Nhiệt<br>(°C)</th><th>BME Ẩm<br>(%)</th><th>BME Áp suất<br>(hPa)</th>";
  html += "<th>BMP Nhiệt<br>(°C)</th><th>BMP Áp suất<br>(hPa)</th>";
  html += "<th>HTU Nhiệt<br>(°C)</th><th>HTU Ẩm<br>(%)</th>";
  html += "<th>SHT Nhiệt<br>(°C)</th><th>SHT Ẩm<br>(%)</th>";
  html += "<th>AHT Nhiệt<br>(°C)</th><th>AHT Ẩm<br>(%)</th></tr>";

  html += "<tr>";
  html += "<td><strong>" + currentDate + "</strong></td><td><strong>" + currentTime + "</strong></td>";
  html += "<td>" + String(pm1) + "</td><td><strong style='color:#e74c3c;'>" + String(pm25) + "</strong></td><td>" + String(pm10) + "</td>";
  html += "<td>" + String(t_bme, 2) + "</td><td>" + String(h_bme, 2) + "</td><td>" + String(p_bme, 2) + "</td>";
  html += "<td>" + String(t_bmp, 2) + "</td><td>" + String(p_bmp, 2) + "</td>";
  html += "<td>" + String(t_htu, 2) + "</td><td>" + String(h_htu, 2) + "</td>";
  html += "<td>" + String(t_sht, 2) + "</td><td>" + String(h_sht, 2) + "</td>";
  html += "<td>" + String(t_aht, 2) + "</td><td>" + String(h_aht, 2) + "</td>";
  html += "</tr>";
  html += "</table></div>";

  // KHỐI HIỂN THỊ DUNG LƯỢNG THẺ SD
  html += "<div style='margin: 25px auto; max-width: 500px; padding: 20px; background: #eef2f3; border-radius: 8px; border: 1px solid #bdc3c7;'>";
  html += "<h3 style='margin-top:0; text-align:center; color:#2c3e50;'>💾 TRẠNG THÁI BỘ NHỚ THẺ SD</h3>";
  
  html += "<div style='display:flex; justify-content:space-between; font-size:14px; margin-bottom:5px;'>";
  html += "<span><b>Đã dùng:</b> <span style='color:#e74c3c;'>" + String(usedMB, 2) + " MB</span></span>";
  html += "<span><b>Còn trống:</b> <span style='color:#27ae60;'>" + String(freeMB, 1) + " MB</span></span>";
  html += "</div>";

  // Vẽ thanh Progress Bar
  html += "<div style='width:100%; background-color:#dcdde1; border-radius:10px; overflow:hidden; border: 1px solid #95a5a6;'>";
  html += "<div style='height:15px; background-color:#e74c3c; width:" + String(percentUsed, 1) + "%; text-align:center; color:white; font-size:10px; line-height:15px;'>" + String(percentUsed, 1) + "%</div>";
  html += "</div>";
  
  html += "<p style='text-align:center; font-size:13px; margin-top:8px;'>Tổng dung lượng Format thực tế: <b>" + String(totalMB, 1) + " MB</b></p>";
  html += "</div>";

  // CÁC NÚT TẢI & XÓA
  html += "<a href='/download' class='btn'>📥 TẢI XUỐNG FILE EXCEL (.CSV)</a>";
  html += "<a href='/delete' class='btn-danger' onclick=\"return confirm('CẢNH BÁO: Bấm OK sẽ XÓA TOÀN BỘ DỮ LIỆU CŨ trong thẻ SD.\\n\\nBạn có chắc chắn muốn xóa không?');\">🗑️ XÓA TOÀN BỘ LỊCH SỬ ĐỂ TẠO LẠI FILE MỚI</a>";
  
  html += "</div></body></html>";
  server.send(200, "text/html", html);
}

void handleDownload() {
  File downloadFile = SD.open("/datalog.csv", FILE_READ);
  
  if (!downloadFile) {
    server.send(404, "text/plain", "Loi: File datalog.csv chua duoc tao! Ban hay doi vai giay de may ghi du lieu roi thu lai.");
    return;
  }
  
  server.sendHeader("Content-Type", "text/csv");
  server.sendHeader("Content-Disposition", "attachment; filename=datalog.csv");
  server.sendHeader("Connection", "close");
  server.streamFile(downloadFile, "application/octet-stream");
  downloadFile.close();
}
void handleDelete() {
  if (SD.exists("/datalog.csv")) {
    SD.remove("/datalog.csv");
    // Báo xóa thành công và tự động Refresh lùi về trang chủ
    server.send(200, "text/html", "<meta charset='UTF-8'><script>alert('Da xoa thanh cong! He thong se khoi tao file CSV moi.'); window.location.href='/';</script>");
  } else {
    server.send(404, "text/html", "<meta charset='UTF-8'><script>alert('Khong tim thay file datalog.csv tren the nho de xoa!'); window.location.href='/';</script>");
  }
}

