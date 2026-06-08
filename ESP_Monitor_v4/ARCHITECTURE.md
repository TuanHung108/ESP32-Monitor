# ESP_Monitor_v4 Architecture

## Overview

`ESP_Monitor_v4` là phiên bản firmware “monolithic” (chạy theo 1 sketch lớn) cho ESP32 để:

- Đọc nhiều cảm biến môi trường:
  - PMS7003 (UART)
  - SDS011 (UART)
  - BME280, BMP280, HTU21D, SHT31, AHT20 (I2C)
- Đồng hồ thời gian thực:
  - RTC DS3231
  - Đồng bộ NTP (UTC+7)
- Ghi dữ liệu ra thẻ SD thành file CSV (`/datalog.csv`)
- Cung cấp dashboard web đơn giản qua `WebServer`:
  - Trang `/` hiển thị dữ liệu và trạng thái SD
  - `/download` để tải file CSV
  - `/delete` để xóa lịch sử (có mật khẩu)
  - `/settime` để set RTC từ PC/điện thoại
- Hiển thị OLED (SSD1306) và “cuộn trang” theo vòng.

## File Structure

Trong folder v4 chỉ có một file chính:

- `ESP_Monitor_v4.ino`
  - Khai báo toàn bộ biến global, cấu hình phần cứng, hàm xử lý web, hàm đọc cảm biến, ghi SD, vẽ OLED.

## Main Control Flow

### `setup()`
1. Khởi tạo Serial debug.
2. Khởi tạo I2C (`Wire.begin(21,22)`) và tốc độ bus.
3. Khởi tạo UART cho:
   - PMS7003 trên `Serial2` (RX=16, TX=17)
   - SDS011 trên `Serial1` (RX=26, TX=27)
4. Khởi tạo OLED.
5. Khởi tạo SD (`SD.begin(SD_CS_PIN)`).
6. Khởi tạo WiFi và kết nối AP.
7. Đăng ký routes web:
   - `/` → `handleRoot`
   - `/download` → `handleDownload`
   - `/delete` → `handleDelete`
   - `/settime` → `handleSetTime`
8. Khởi tạo RTC và các cảm biến I2C (HTU21D, BME280, BMP280, AHT20, SHT31).
9. Cấu hình SDS011 active reporting mode.

### `loop()`
- `server.handleClient()` luôn lắng nghe web.
- NTP:
  - Đồng bộ lần đầu sau 5s boot.
  - Đồng bộ định kỳ mỗi 6 giờ.
- PMS7003:
  - Đọc PMS liên tục theo `pms.read(data)` để cập nhật `pm1/pm25/pm10`.
- Mỗi 5 giây (`interval=5000`):
  1. `readSensors()` (đọc SDS + các cảm biến I2C + cập nhật `currentDate/currentTime`)
  2. `logToSDCard()` (ghi CSV, tự tạo header nếu file chưa tồn tại)
  3. `updateOLED()` (vẽ page theo `oledPage`)
  4. Serial print log trạng thái.

## Data & CSV Format

File CSV: `/datalog.csv`
- Có BOM UTF-8 để hỗ trợ tiếng Việt font.
- Có header khi file chưa tồn tại.
- Một dòng dữ liệu được format:

`Ngày đo;Giờ đo;PMS_PM1.0;PMS_PM2.5;PMS_PM10;SDS_PM2.5;SDS_PM10;BME_Nhiệt;BME_Ẩm;BME_Áp suất;BMP_Nhiệt;BMP_Áp suất;HTU_Nhiệt;HTU_Ẩm;SHT_Nhiệt;SHT_Ẩm;AHT_Nhiệt;AHT_Ẩm`

## Sensor Validation & Error Recovery

- Mỗi cảm biến I2C có hàm validate:
  - Nhiệt độ: `-20..80`
  - Độ ẩm: `0..100`
  - Áp suất: `800..1200`
- Nếu giá trị không hợp lệ:
  - Không trả về ngay, mà thực hiện **re-init theo cooldown 30 giây** cho từng cảm biến (BME/BMP/HTU/SHT/AHT).

## Web API / Routes

### `GET /`
- Trả HTML trang dashboard hiển thị:
  - Dữ liệu cảm biến hiện tại
  - Thông tin dung lượng SD (Tổng/Đã dùng/Còn trống/Percent)
  - Nút `/download` và nút xóa `/delete`
  - Nút đồng bộ thời gian `/settime`

### `GET /download`
- Stream file `/datalog.csv` về client.

### `GET /delete?pass=delete123`
- Xóa file `/datalog.csv` nếu đúng mật khẩu.

### `GET /settime?t=<unix_ts>`
- Set RTC theo Unix timestamp nhận từ client.

## Architectural Notes (so với v6)

- v4 là single-sketch kiểu “monolithic”: toàn bộ logic nằm chung trong `.ino`.
- v4 dùng nhiều `String` trong trang web và hiển thị (khác với v6 ưu tiên char buffers).
- v6 tách layer rõ ràng (Application/Service/Driver) còn v4 thì không.

## Version

- `ESP_Monitor_v4 Architecture` (tài liệu tóm tắt)

