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
#include <SdsDustSensor.h> // <-- Thêm thư viện SDS011
#include <time.h>

// ---------------- KHAI BÁO HÀM THUỘC TÍNH ----------------
void handleRoot();
void handleDownload();
void handleDelete();
void handleSetTime();
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

// Khởi tạo SDS011 trên Serial1 của ESP32
SdsDustSensor sds(Serial1); 

// ---------------- BIẾN TOÀN CỤC CHỨA DỮ LIỆU --------
int pm1 = 0, pm25 = 0, pm10 = 0; // Dữ liệu của PMS7003
float sds_pm25 = 0.0, sds_pm10 = 0.0; // <-- Dữ liệu của SDS011
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

// ---------------- RE-INIT TIMER ----------------
unsigned long lastBMEReinit = 0;
unsigned long lastBMPReinit = 0;
unsigned long lastHTUReinit = 0;
unsigned long lastSHTReinit = 0;
unsigned long lastAHTReinit = 0;

const unsigned long REINIT_COOLDOWN = 30000; // 30 giây

// ---------------- CHECK VALID ----------------
bool validTemperature(float t) {
  return (t > -20 && t < 80);
}

bool validHumidity(float h) {
  return (h >= 0 && h <= 100);
}

bool validPressure(float p) {
  return (p > 800 && p < 1200);
}


void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  Wire.setClock(100000);
  delay(200);
  
  Serial2.begin(9600, SERIAL_8N1, 16, 17); // PMS7003
  Serial1.begin(9600, SERIAL_8N1, 26, 27); // SDS011 (RX=26, TX=27)

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
  server.on("/delete", handleDelete); 
  server.on("/settime", handleSetTime);
  server.begin();

  // 5. Khởi tạo Cảm biến
  if (!rtc.begin()) Serial.println("Loi RTC");

  if (!htu.begin()) Serial.println("Loi HTU21D");
  if (!bme.begin(0x76)) Serial.println("Loi BME280(0x76)");
  if (!bmp.begin(0x77)) Serial.println("Loi BMP280(0x77)"); // Chân SDO nối 3.3V
  if (!aht.begin()) Serial.println("Loi AHT20");
  if (!sht.begin()) Serial.println("Loi SHT31");
  
  // Khởi động giao tiếp thuật toán của thư viện SDS011
  sds.begin(); 
  Serial.println(sds.queryFirmwareVersion().toString()); // In thông tin phần mềm SDS011
  sds.setActiveReportingMode(); // Đặt chế độ chủ động gửi gói tin liên tục
}

void loop() {
  // Máy chủ Web luôn lắng nghe
  server.handleClient();

  // ================= NTP AUTO =================
  if (WiFi.status() == WL_CONNECTED) {

    // Sync lần đầu sau 5s
    if (!ntpDoneOnce && millis() > 5000) {
      Serial.println("NTP lan dau...");
      syncTimeWithNTP();
      ntpDoneOnce = true;
      lastNtpSync = millis();
    }

    // Sync mỗi 6 giờ
    if (millis() - lastNtpSync > ntpInterval) {
      if (millis() - previousMillis > 1000) { // tránh trùng lúc ghi SD
        Serial.println("NTP dinh ky...");
        syncTimeWithNTP();
        lastNtpSync = millis();
      }
    }
  }
  
  // Bắt gói tin PMS7003 liên tục
  if (pms.read(data)) {
    pm1 = data.PM_AE_UG_1_0;
    pm25 = data.PM_AE_UG_2_5;
    pm10 = data.PM_AE_UG_10_0;
  }

  // Đếm thời gian 5 giây 1 nhịp
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    readSensors();
    logToSDCard();
    updateOLED();
    
    Serial.println("[" + currentTime + "] IP: " + WiFi.localIP().toString() + " | SD Logged");
  }
}

// ================= CÁC HÀM XỬ LÝ =================
void syncTimeWithNTP() {

  static bool ntpConfigured = false;

  if (!ntpConfigured) {
    configTime(7*3600, 0, "pool.ntp.org");
    ntpConfigured = true;
  }

  Serial.print("Dong bo NTP");

  time_t now = time(nullptr);
  int retry = 0;

  while (now < 1000000000 && retry < 10) {
    delay(200);
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

  Serial.println("RTC updated (NTP)");
}


void readSensors() {

  DateTime now = rtc.now();

  char dateBuf[20];
  sprintf(dateBuf, "%04d/%02d/%02d",
          now.year(), now.month(), now.day());
  currentDate = String(dateBuf);

  char timeBuf[20];
  sprintf(timeBuf, "%02d:%02d:%02d",
          now.hour(), now.minute(), now.second());
  currentTime = String(timeBuf);

  unsigned long nowMs = millis();

  // ================= SDS011 =================
  PmResult sds_res = sds.readPm();
  if (sds_res.isOk()) {
    sds_pm25 = sds_res.pm25;
    sds_pm10 = sds_res.pm10;
  } else {
    Serial.println("SDS011 ERROR: " + sds_res.statusToString());
  }

  // ================= BME280 =================
  float tb = bme.readTemperature();
  float hb = bme.readHumidity();
  float pb = bme.readPressure() / 100.0F;

  t_bme = tb;
  h_bme = hb;
  p_bme = pb;

  if (!(validTemperature(tb) &&
        validHumidity(hb) &&
        validPressure(pb))) {

    Serial.println("BME280 ERROR");

    if (nowMs - lastBMEReinit > REINIT_COOLDOWN) {

      Serial.println("BME280 -> Re-init");

      if (bme.begin(0x76)) {
        Serial.println("BME280 Re-init OK");
      } else {
        Serial.println("BME280 Re-init FAILED");
      }

      lastBMEReinit = nowMs;
    }
  }

  delay(5);

  // ================= BMP280 =================
  float tbmp = bmp.readTemperature();
  float pbmp = bmp.readPressure() / 100.0F;

  t_bmp = tbmp;
  p_bmp = pbmp;

  if (!(validTemperature(tbmp) &&
        validPressure(pbmp))) {

    Serial.println("BMP280 ERROR");

    if (nowMs - lastBMPReinit > REINIT_COOLDOWN) {

      Serial.println("BMP280 -> Re-init");

      if (bmp.begin(0x77)) {
        Serial.println("BMP280 Re-init OK");
      } else {
        Serial.println("BMP280 Re-init FAILED");
      }

      lastBMPReinit = nowMs;
    }
  }

  delay(5);

  // ================= HTU21D =================
  float thtu = htu.readTemperature();
  float hhtu = htu.readHumidity();

  t_htu = thtu;
  h_htu = hhtu;

  if (!(validTemperature(thtu) &&
        validHumidity(hhtu))) {

    Serial.println("HTU21D ERROR");

    if (nowMs - lastHTUReinit > REINIT_COOLDOWN) {

      Serial.println("HTU21D -> Re-init");

      if (htu.begin()) {
        Serial.println("HTU21D Re-init OK");
      } else {
        Serial.println("HTU21D Re-init FAILED");
      }

      lastHTUReinit = nowMs;
    }
  }

  delay(5);

  // ================= SHT31 =================
  float tsht = sht.readTemperature();
  float hsht = sht.readHumidity();

  t_sht = tsht;
  h_sht = hsht;

  if (!(validTemperature(tsht) &&
        validHumidity(hsht))) {

    Serial.println("SHT31 ERROR");

    if (nowMs - lastSHTReinit > REINIT_COOLDOWN) {

      Serial.println("SHT31 -> Re-init");

      if (sht.begin(0x44)) {
        Serial.println("SHT31 Re-init OK");
      } else {
        Serial.println("SHT31 Re-init FAILED");
      }

      lastSHTReinit = nowMs;
    }
  }

  delay(5);

  // ================= AHT20 =================
  sensors_event_t h_event, t_event;

  aht.getEvent(&h_event, &t_event);

  float taht = t_event.temperature;
  float haht = h_event.relative_humidity;

  t_aht = taht;
  h_aht = haht;

  if (!(validTemperature(taht) &&
        validHumidity(haht))) {

    Serial.println("AHT20 ERROR");

    if (nowMs - lastAHTReinit > REINIT_COOLDOWN) {

      Serial.println("AHT20 -> Re-init");

      if (aht.begin()) {
        Serial.println("AHT20 Re-init OK");
      } else {
        Serial.println("AHT20 Re-init FAILED");
      }

      lastAHTReinit = nowMs;
    }
  }

  delay(5);
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
      // Đã thêm tiêu đề cột cho SDS011 (SDS_PM2.5 và SDS_PM10)
      dataFile.println("Ngày đo;Giờ đo;PMS_PM1.0 (µg/m³);PMS_PM2.5 (µg/m³);PMS_PM10 (µg/m³);SDS_PM2.5 (µg/m³);SDS_PM10 (µg/m³);BME_Nhiệt (°C);BME_Ẩm (%);BME_Áp suất (hPa);BMP_Nhiệt (°C);BMP_Áp suất (hPa);HTU_Nhiệt (°C);HTU_Ẩm (%);SHT_Nhiệt (°C);SHT_Ẩm (%);AHT_Nhiệt (°C);AHT_Ẩm (%)");
    }
    
    // Xuất chuỗi định dạng bao gồm cả giá trị SDS011
    dataFile.printf("%s;%s;%d;%d;%d;%.1f;%.1f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f\n",
      currentDate.c_str(), currentTime.c_str(), pm1, pm25, pm10, sds_pm25, sds_pm10, 
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
    display.println("[Page 1] Dust Compare");
    display.printf("PMS25:%d|SDS25:%.1f\n", pm25, sds_pm25);
    display.printf("PMS10:%d|SDS10:%.1f\n", pm10, sds_pm10);
    display.printf("PMS1.0: %d ug/m3\n", pm1);
  } 
  else if (oledPage == 1) {
    display.println("[Page 2]");
    display.printf("BME T:%.1fC, Ap:%.0f\n", t_bme, p_bme);
    display.printf("BME H:%.1f%%\n", h_bme);
    display.printf("BMP T:%.1fC, Ap:%.0f\n", t_bmp, p_bmp);
  }
  else if (oledPage == 2) {
    display.println("[Page 3]");
    display.printf("HTU T:%.1fC, H:%.1f%%\n", t_htu, h_htu);
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
  
  // BẢNG BÁO CÁO THÔNG SỐ (Bổ sung thêm 2 cột cho SDS011)
  html += "<div class='table-wrapper'><table>";
  html += "<tr><th>Ngày đo</th><th>Giờ Đo</th><th>PMS PM1.0<br>(µg/m³)</th><th>PMS PM2.5<br>(µg/m³)</th><th>PMS PM10<br>(µg/m³)</th>";
  html += "<th>SDS PM2.5<br>(µg/m³)</th><th>SDS PM10<br>(µg/m³)</th>";
  html += "<th>BME Nhiệt<br>(°C)</th><th>BME Ẩm<br>(%)</th><th>BME Áp suất<br>(hPa)</th>";
  html += "<th>BMP Nhiệt<br>(°C)</th><th>BMP Áp suất<br>(hPa)</th>";
  html += "<th>HTU Nhiệt<br>(°C)</th><th>HTU Ẩm<br>(%)</th>";
  html += "<th>SHT Nhiệt<br>(°C)</th><th>SHT Ẩm<br>(%)</th>";
  html += "<th>AHT Nhiệt<br>(°C)</th><th>AHT Ẩm<br>(%)</th></tr>";

  html += "<tr>";
  html += "<td><strong>" + currentDate + "</strong></td><td><strong>" + currentTime + "</strong></td>";
  html += "<td>" + String(pm1) + "</td><td><strong style='color:#e74c3c;'>" + String(pm25) + "</strong></td><td>" + String(pm10) + "</td>";
  html += "<td><strong style='color:#2980b9;'>" + String(sds_pm25, 1) + "</strong></td><td>" + String(sds_pm10, 1) + "</td>";
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
  html += R"rawliteral(
  <button class='btn-danger' onclick='confirmDelete()'>
  🗑️ XÓA TOÀN BỘ LỊCH SỬ
  </button>

  <script>
  function confirmDelete() {

    let pass = prompt(
      "CANH BAO!\n\n" +
      "Nhap mat khau de xoa du lieu:"
    );

    if(pass === null) {
      return;
    }

    let ok = confirm(
      "BAN CHAC CHAN MUON XOA TOAN BO DU LIEU?"
    );

    if(ok) {
      window.location.href='/delete?pass=' + pass;
    }
  }
  </script>
  )rawliteral";

  // CÁC NÚT CHỈNH THỜI GIAN ĐỒNG BỘ VỚI PC & ĐIỆN THOẠI
  html += "<script>";
  html += "function syncMyTime() {";
  html += "  var now = Math.floor(Date.now() / 1000);"; 
  html += "  var gmt7 = now + (7 * 3600);"; // Cộng thêm 7 tiếng cho Việt Nam
  html += "  fetch('/settime?t=' + gmt7).then(r => { if(r.ok) alert('Đã đồng bộ giờ GMT+7 thành công!'); });";
  html += "}";
  html += "</script>";
  html += "<button onclick='syncMyTime()' class='btn' style='background-color:#f39c12; border:none; cursor:pointer;'>🕒 ĐỒNG BỘ GIỜ TỪ ĐIỆN THOẠI / PC</button>";

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

  if (!server.hasArg("pass")) {

    server.send(
      403,
      "text/html",
      "<script>alert('Thieu mat khau!');window.location.href='/';</script>"
    );

    return;
  }

  String pass = server.arg("pass");

  if (pass != "delete123") {

    server.send(
      403,
      "text/html",
      "<script>alert('Sai mat khau!');window.location.href='/';</script>"
    );

    return;
  }

  if (SD.exists("/datalog.csv")) {

    SD.remove("/datalog.csv");

    server.send(
      200,
      "text/html",
      "<meta charset='UTF-8'>"
      "<script>"
      "alert('Da xoa thanh cong!');"
      "window.location.href='/';"
      "</script>"
    );

  } else {

    server.send(
      404,
      "text/html",
      "<script>"
      "alert('Khong tim thay file!');"
      "window.location.href='/';"
      "</script>"
    );
  }
}

void handleSetTime() {
  if (server.hasArg("t")) {
    uint32_t ts = server.arg("t").toInt();
    rtc.adjust(DateTime(ts));
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Error");
  }
}