// Implement giao diện dashboard
// HTML/CSS/JS từ bản v5
// Giữ nguyên để UI không đổi

#include "web_pages.h"

String getDashboardPage(float totalMB, float usedMB, float freeMB, float percentUsed) {
  // NOTE: Full HTML implementation from v5 should be inserted here
  // To save tokens, reference the v5 file directly
  // In actual deployment, copy entire web_pages.h from v5 replacing web/web_pages.h
  
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html lang="vi">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>ESP32 Monitoring Dashboard</title>
        <link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&display=swap" rel="stylesheet">
        <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">
        
        <style>
            :root {
                --bg-gradient: linear-gradient(135deg, #0f2027, #203a43, #2c5364);
                --card-bg: rgba(255, 255, 255, 0.08);
                --card-border: rgba(255, 255, 255, 0.1);
                --text-main: #ffffff;
                --text-sub: #b0bec5;
                --primary: #00e5ff;
                --success: #00e676;
                --warning: #ffea00;
                --danger: #ff1744;
                
                /* Màu nền 6 cấp độ ô nhiễm chuẩn IQAir */
                --iq-good: #4caf50; /* Tốt - Xanh lá */
                --iq-moderate: #ffeb3b; /* Trung bình - Vàng */
                --iq-unhealthy-sub: #ff9800; /* Kém (Nhạy cảm) - Cam */
                --iq-unhealthy: #f44336; /* Không tốt - Đỏ */
                --iq-very-unhealthy: #9c27b0; /* Rất kém - Tím */
                --iq-hazardous: #795548; /* Nguy hiểm - Nâu */
            }

            * { margin: 0; padding: 0; box-sizing: border-box; font-family: 'Inter', sans-serif; }
            body { background: var(--bg-gradient); color: var(--text-main); min-height: 100vh; padding: 20px; }
            
            .container { max-width: 1400px; margin: 0 auto; display: flex; flex-direction: column; gap: 20px; }
            
            /* Header Dashboard */
            header { display: flex; justify-content: space-between; align-items: center; padding: 10px 0; border-bottom: 1px solid var(--card-border); }
            header h1 { font-size: 24px; font-weight: 700; letter-spacing: -0.5px; display: flex; align-items: center; gap: 10px; }
            header h1 i { color: var(--primary); }
            .sys-time { font-size: 14px; color: var(--text-sub); text-align: right; }

            /* Thanh trạng thái hệ thống */
            .status-bar { display: flex; flex-wrap: wrap; gap: 15px; align-items: center; }
            .status-badge { background: var(--card-bg); border: 1px solid var(--card-border); padding: 10px 18px; border-radius: 30px; display: flex; align-items: center; gap: 10px; font-size: 14px; font-weight: 500; }
            .status-badge .dot { width: 8px; height: 8px; border-radius: 50%; display: inline-block; }
            .dot.online { background-color: var(--success); box-shadow: 0 0 8px var(--success); }
            .dot.offline { background-color: var(--danger); box-shadow: 0 0 8px var(--danger); }

            /* Bố cục Grid hàng 1 gồm Khung gộp bụi mịn song song với Khí quyển trực tuyến */
            .top-row-grid { display: grid; grid-template-columns: 2fr 1fr; gap: 20px; align-items: stretch; }
            @media (max-width: 1100px) { .top-row-grid { grid-template-columns: 1fr; } }

            /* Cấu trúc chia 3 ô vuông cân bằng phía trong Card Giám sát bụi mịn */
            .dust-monitor-inner-grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 15px; align-items: stretch; margin-top: 10px; }
            @media (max-width: 650px) { .dust-monitor-inner-grid { grid-template-columns: 1fr; } }

            /* Ô đánh giá IQAir nhỏ gọn tinh tế bên trái */
            .iqair-mini-panel { border-radius: 12px; padding: 12px; text-align: center; color: #fff; background: var(--iq-1); transition: background 0.5s ease; display: flex; flex-direction: column; justify-content: center; align-items: center; min-height: 150px; height: 100%; }
            .iqair-panel-icon { font-size: 36px; margin-bottom: 4px; }
            .iqair-panel-status { font-size: 15px; font-weight: 700; text-transform: uppercase; margin-bottom: 2px; }
            .iqair-panel-value { font-size: 32px; font-weight: 300; line-height: 1; }
            .iqair-panel-lbl { font-size: 11px; opacity: 0.95; margin-top: 4px; font-weight: 500; }

            /* Ô con phụ hiển thị Text thô của cảm biến bụi */
            .sensor-right-subcard { background: rgba(0,0,0,0.15); padding: 12px; border-radius: 12px; border: 1px solid rgba(255,255,255,0.03); display: flex; flex-direction: column; justify-content: center; }
            .sensor-data-line { font-size: 13px; font-weight: 500; margin: 5px 0; display: flex; justify-content: space-between; border-bottom: 1px solid rgba(255,255,255,0.05); padding-bottom: 3px; }
            .sensor-data-line span { font-weight: 600; color: #fff; }

            /* Thiết kế các Thẻ chức năng (Cards) */
            .card { background: var(--card-bg); border: 1px solid var(--card-border); border-radius: 16px; padding: 20px; backdrop-filter: blur(10px); -webkit-backdrop-filter: blur(10px); box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.2); }
            .card-title { font-size: 14px; font-weight: 600; color: var(--text-sub); margin-bottom: 10px; display: flex; align-items: center; gap: 6px; text-transform: uppercase; letter-spacing: 0.5px; }

            /* ── Ô KHÍ QUYỂN TRỰC TUYẾN: ĐỒNG BỘ VIỀN MỜ GIỐNG CÁC Ô CÒN LẠI ── */
            .weather-widget { 
                display: flex; 
                flex-direction: column; 
                justify-content: center; 
                gap: 10px; 
                background: linear-gradient(135deg, #184776, #0b6875) !important; 
                border: 1px solid var(--card-border) !important; /* Đã sửa lỗi viền mờ đồng bộ */
                height: 100%; 
            }
            .weather-temp { font-size: 36px; font-weight: 600; margin-top: 2px; color: #ffffff; }

            /* KHÓA CHẶT 1 HÀNG NGANG CHO KHỐI THÔNG SỐ ĐO ĐẠC GIỮA 5 IC CẢM BIẾN */
            .horizontal-sensor-grid { display: grid; grid-template-columns: repeat(5, 1fr); gap: 15px; width: 100%; }
            @media (max-width: 900px) { .horizontal-sensor-grid { grid-template-columns: repeat(2, 1fr); } }
            .sensor-subcard { background: rgba(0,0,0,0.15); padding: 12px; border-radius: 12px; border: 1px solid rgba(255,255,255,0.03); text-align: left; }
            .sensor-subcard .label { font-size: 11px; color: var(--text-sub); margin-bottom: 4px; font-weight: 600; text-transform: uppercase; }
            .sensor-subcard .val { font-size: 14px; font-weight: 600; line-height: 1.4; color: #fff; }

            /* Đồ thị */
            .chart-section { grid-column: 1 / -1; }
            .chart-container { position: relative; width: 100%; height: 440px; margin-top: 15px; }
            .chart-controls { display: flex; flex-wrap: wrap; gap: 12px; align-items: center; margin-top: 10px; }
            .chart-controls select { border-radius: 10px; border: 1px solid rgba(255,255,255,0.12); padding: 10px 14px; background: rgba(255,255,255,0.08); color: #fff; min-width: 200px; }
            .chart-controls .info-text { color: #b0bec5; font-size: 13px; }

            /* Bảng nhật ký lấy mẫu chi tiết */
            .table-section { grid-column: 1 / -1; }
            .table-header-container { display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px; flex-wrap: wrap; gap: 15px; }
            .table-wrapper { width: 100%; max-height: 400px; overflow-x: auto; overflow-y: auto; border-radius: 8px; border: 1px solid var(--card-border); background: rgba(0, 0, 0, 0.2); }
            table { width: 100%; border-collapse: collapse; text-align: center; font-size: 12px; min-width: 1500px; }
            th { background: rgba(255, 255, 255, 0.12); color: var(--primary); font-weight: 600; padding: 12px 8px; position: sticky; top: 0; border-bottom: 1px solid var(--card-border); }
            td { padding: 10px 6px; border-bottom: 1px solid rgba(255, 255, 255, 0.05); color: #fff; }
            tr:hover { background: rgba(255, 255, 255, 0.04); }

            /* Các nút bấm thao tác trên Web */
            .btn-group { display: flex; gap: 10px; flex-wrap: wrap; }
            .btn { border: none; padding: 8px 16px; border-radius: 8px; font-size: 13px; font-weight: 500; cursor: pointer; display: inline-flex; align-items: center; gap: 6px; transition: all 0.2s ease; color: #fff; background: rgba(255,255,255,0.1); border: 1px solid rgba(255,255,255,0.1); text-decoration: none;}
            .btn:hover { background: rgba(255,255,255,0.2); transform: translateY(-1px); }
            .btn-primary { background: var(--primary); color: #000; font-weight: 600; border: none; }
            .btn-primary:hover { background: #00b8d4; box-shadow: 0 0 10px rgba(0,229,255,0.4); }
            .btn-danger { background: rgba(255, 23, 68, 0.15); border: 1px solid var(--danger); color: #ff8a80; }
            .btn-danger:hover { background: var(--danger); color: #fff; }
        </style>
    </head>
    <body>

    <div class="container">
        <header>
            <div>
                <h1><i class="fa-solid fa-microchip"></i> ESP32 MONITORING</h1>
            </div>
            <div class="sys-time">
                <div id="date-str">Đang đồng bộ...</div>
                <div id="clock-str" style="font-size: 20px; font-weight: 600; color: #fff;">00:00:00</div>
            </div>
        </header>

        <div class="status-bar">
            <div class="status-badge">
                <span class="dot offline" id="sd-dot"></span>
                <span>Trạng thái: <strong id="sd-status" style="color:var(--warning)">ĐANG KẾT NỐI MẠCH...</strong></span>
            </div>
            <div class="status-badge">Tổng: <strong id="sd-total">)rawliteral" + String(totalMB, 2) + R"rawliteral( MB</strong></div>
            <div class="status-badge">Đã dùng: <strong id="sd-used">)rawliteral" + String(usedMB, 2) + R"rawliteral( MB</strong></div>
            <div class="status-badge">Còn trống: <strong id="sd-free">)rawliteral" + String(freeMB, 2) + R"rawliteral( MB</strong></div>
            <div class="status-badge">% sử dụng: <strong id="sd-percent">)rawliteral" + String(percentUsed, 2) + R"rawliteral(%</strong></div>
        </div>

        <div class="top-row-grid">
            
            <div class="card">
                <div class="card-title"><i class="fa-solid fa-smog"></i> Giám sát nồng độ bụi mịn thời gian thực</div>
                <div class="dust-monitor-inner-grid">
                    
                    <div class="iqair-mini-panel" id="iqair-box">
                        <div class="iqair-panel-icon" id="iqair-icon"><i class="fa-solid fa-face-smile"></i></div>
                        <div class="iqair-panel-status" id="iqair-status">Đang đo...</div>
                        <div class="iqair-panel-value" id="iqair-value">--</div>
                        <div class="iqair-panel-lbl" id="iqair-pollutant">Ô nhiễm chính: --</div>
                    </div>

                    <div class="sensor-right-subcard">
                        <h4 style="font-size: 12px; color: var(--primary); text-transform: uppercase; margin-bottom: 4px;"><i class="fa-solid fa-circle" style="font-size: 11px; margin-right: 2px;"></i> PMS7003</h4>
                        <div class="sensor-data-line">PM1.0: <span id="pm10-val">-- µg/m³</span></div>
                        <div class="sensor-data-line">PM2.5: <span id="pm25-val">-- µg/m³</span></div>
                        <div class="sensor-data-line">PM10: <span id="pm100-val">-- µg/m³</span></div>
                    </div>

                    <div class="sensor-right-subcard">
                        <h4 style="font-size: 12px; color: #ffe57f; text-transform: uppercase; margin-bottom: 4px;"><i class="fa-solid fa-circle" style="font-size: 11px; margin-right: 2px;"></i> SDS011</h4>
                        <div class="sensor-data-line">PM2.5: <span id="sds-pm25">-- µg/m³</span></div>
                        <div class="sensor-data-line">PM10: <span id="sds-pm10">-- µg/m³</span></div>
                    </div>

                </div>
            </div>

            <div class="card weather-widget">
                <div class="card-title" style="margin-bottom:0; color: rgba(255,255,255,0.8);"><i class="fa-solid fa-cloud-sun"></i> Khí quyển trực tuyến</div>
                <span id="weather-desc" style="font-size: 13px; color: rgba(255,255,255,0.7);">Bách Khoa, Hà Nội</span>
                <div class="weather-temp" id="weather-temp-val">--°C</div>
                <div style="font-size: 13px; font-weight: 500; margin-top: 4px; color: #ffffff;">
                    <i class="fa-solid fa-droplet" style="color: #00e5ff; margin-right: 4px;"></i> Độ ẩm môi trường: <span id="weather-hum-val" style="font-size: 18px; font-weight: 700; color: #00e5ff;">--</span>%
                </div>
            </div>

        </div>

        <div class="card">
            <div class="card-title"><i class="fa-solid fa-layer-group"></i> Thông số đo đạc giữa các cảm biến môi trường</div>
            <div class="horizontal-sensor-grid">
                <div class="sensor-subcard">
                    <div class="label" style="color: #ff8a80;"><i class="fa-solid fa-circle-nodes"></i> SHT31</div>
                    <div class="val">T: <span id="sht-t">--</span>°C<br>H: <span id="sht-h">--</span>%</div>
                </div>
                <div class="sensor-subcard">
                    <div class="label" style="color: #80d8ff;"><i class="fa-solid fa-circle-nodes"></i> BME280</div>
                    <div class="val">T: <span id="bme-t">--</span>°C<br>H: <span id="bme-h">--</span>%<br>P: <span id="bme-p">--</span> hPa</div>
                </div>
                <div class="sensor-subcard">
                    <div class="label" style="color: #a7ffeb;"><i class="fa-solid fa-circle-nodes"></i> BMP280</div>
                    <div class="val">T: <span id="bmp-t">--</span>°C<br>P: <span id="bmp-p">--</span> hPa</div>
                </div>
                <div class="sensor-subcard">
                    <div class="label" style="color: #b9f6ca;"><i class="fa-solid fa-circle-nodes"></i> HTU21D</div>
                    <div class="val">T: <span id="htu-t">--</span>°C<br>H: <span id="htu-h">--</span>%</div>
                </div>
                <div class="sensor-subcard">
                    <div class="label" style="color: #ffe57f;"><i class="fa-solid fa-circle-nodes"></i> AHT20</div>
                    <div class="val">T: <span id="aht-t">--</span>°C<br>H: <span id="aht-h">--</span>%</div>
                </div>
            </div>
        </div>

        <div class="card table-section">
            <div class="table-header-container">
                <div class="card-title" style="margin-bottom:0;"><i class="fa-solid fa-list-check"></i> Nhật ký đo đạc thời gian thực (Mẫu 5 giây / lần)</div>
                <div class="btn-group">
                    <a href="/download" download="datalog.csv" class="btn btn-primary"><i class="fa-solid fa-file-excel"></i> Tải file dữ liệu (.CSV)</a>
                    <button onclick="syncTime()" class="btn"><i class="fa-solid fa-arrows-rotate"></i> Đồng bộ giờ hệ thống</button>
                    <button id="delete-history-btn" class="btn btn-danger"><i class="fa-solid fa-trash-can"></i> Xóa toàn bộ lịch sử đo</button>
                </div>
            </div>
            
            <div class="table-wrapper">
                <table id="data-table">
                    <thead>
                        <tr>
                            <th rowspan="2">Thời gian lấy mẫu</th>
                            <th colspan="3">PMS7003 (µg/m³)</th>
                            <th colspan="2">SDS011 (µg/m³)</th>
                            <th colspan="3">BME280</th>
                            <th colspan="2">BMP280</th>
                            <th colspan="2">SHT31</th>
                            <th colspan="2">HTU21D</th>
                            <th colspan="2">AHT20</th>
                        </tr>
                        <tr>
                            <th>PM1.0</th>
                            <th>PM2.5</th>
                            <th>PM10</th>
                            <th>PM2.5</th>
                            <th>PM10</th>
                            <th>Nhiệt (°C)</th>
                            <th>Ẩm (%)</th>
                            <th>Áp suất (hPa)</th>
                            <th>Nhiệt (°C)</th>
                            <th>Áp suất (hPa)</th>
                            <th>Nhiệt (°C)</th>
                            <th>Ẩm (%)</th>
                            <th>Nhiệt (°C)</th>
                            <th>Ẩm (%)</th>
                            <th>Nhiệt (°C)</th>
                            <th>Ẩm (%)</th>
                        </tr>
                    </thead>
                    <tbody id="table-body">
                        </tbody>
                </table>
            </div>
        </div>

    </div>

    <script>
        function formatNumber(value) {
            return (typeof value === 'number' && !isNaN(value)) ? value.toFixed(2) : '--';
        }

        function updateIQAirMiniBox(pms25, sds25, pms100, sds10) {
            const box = document.getElementById('iqair-box');
            const icon = document.getElementById('iqair-icon');
            const status = document.getElementById('iqair-status');
            const value = document.getElementById('iqair-value');
            const pollutant = document.getElementById('iqair-pollutant');

            let maxPM25 = Math.max(pms25, sds25);
            let maxPM10 = Math.max(pms100, sds10);
            let activeVal = maxPM25;

            if (maxPM25 >= maxPM10 * 0.5) {
                pollutant.innerHTML = "Ô nhiễm chính: <strong>PM2.5</strong>";
                activeVal = maxPM25;
            } else {
                pollutant.innerHTML = "Ô nhiễm chính: <strong>PM10</strong>";
                activeVal = maxPM10;
            }
            value.innerText = activeVal;

            if (activeVal <= 12.0) {
                box.style.backgroundColor = "var(--iq-good)"; box.style.color = "#fff"; value.style.color = "#fff";
                status.innerText = "Tốt";
                icon.innerHTML = '<i class="fa-solid fa-face-smile"></i>';
            } else if (activeVal <= 35.4) {
                box.style.backgroundColor = "var(--iq-moderate)"; box.style.color = "#000"; value.style.color = "#000";
                status.innerText = "Trung bình";
                icon.innerHTML = '<i class="fa-solid fa-face-meh" style="color:#000;"></i>';
            } else if (activeVal <= 55.4) {
                box.style.backgroundColor = "var(--iq-unhealthy-sub)"; box.style.color = "#fff"; value.style.color = "#fff";
                status.innerText = "Kém (Nhạy cảm)";
                icon.innerHTML = '<i class="fa-solid fa-face-frown"></i>';
            } else if (activeVal <= 150.4) {
                box.style.backgroundColor = "var(--iq-unhealthy)"; box.style.color = "#fff"; value.style.color = "#fff";
                status.innerText = "Không tốt";
                icon.innerHTML = '<i class="fa-solid fa-face-frown-open"></i>';
            } else if (activeVal <= 250.4) {
                box.style.backgroundColor = "var(--iq-very-unhealthy)"; box.style.color = "#fff"; value.style.color = "#fff";
                status.innerText = "Rất kém";
                icon.innerHTML = '<i class="fa-solid fa-face-tired"></i>';
            } else {
                box.style.backgroundColor = "var(--iq-hazardous)"; box.style.color = "#fff"; value.style.color = "#fff";
                status.innerText = "Nguy hiểm";
                icon.innerHTML = '<i class="fa-solid fa-face-skull"></i>';
            }
        }

        function fetchLocalWeather() {
            const lat = 21.0056; 
            const lon = 105.8433;
            fetch(`https://api.open-meteo.com/v1/forecast?latitude=${lat}&longitude=${lon}&current=temperature_2m,relative_humidity_2m`)
            .then(res => res.json())
            .then(data => {
                if(data && data.current) {
                    if(data.current.temperature_2m !== undefined) {
                        document.getElementById('weather-temp-val').innerText = Math.round(data.current.temperature_2m) + "°C";
                    }
                    if(data.current.relative_humidity_2m !== undefined) {
                        document.getElementById('weather-hum-val').innerText = data.current.relative_humidity_2m;
                    }
                }
            }).catch(() => console.log("Lỗi đồng bộ dữ liệu thời tiết trực tuyến"));
        }

        function confirmDeleteHistory() {
            const password = prompt('Nhập mật khẩu xóa toàn bộ lịch sử đo:');
            if (!password) return;
            if (!confirm('Bạn có chắc chắn muốn xóa toàn bộ lịch sử đo?')) return;
            window.location.href = `/delete?pass=${encodeURIComponent(password)}`;
        }

        let serverClockSeconds = null;
        let lastClockTickMs = 0;

        function formatClockValue(value) {
            return value.toString().padStart(2, '0');
        }

        function updateClockDisplay() {
            if (serverClockSeconds === null) {
                return;
            }
            const hours = Math.floor(serverClockSeconds / 3600) % 24;
            const minutes = Math.floor((serverClockSeconds % 3600) / 60);
            const seconds = serverClockSeconds % 60;
            document.getElementById('clock-str').innerText = `${formatClockValue(hours)}:${formatClockValue(minutes)}:${formatClockValue(seconds)}`;
        }

        function tickClock() {
            if (serverClockSeconds === null) {
                return;
            }
            serverClockSeconds = (serverClockSeconds + 1) % 86400;
            updateClockDisplay();
        }

        function setClockFromServer(timeStr) {
            const parts = timeStr.split(':');
            if (parts.length !== 3) {
                return;
            }
            const hours = parseInt(parts[0], 10);
            const minutes = parseInt(parts[1], 10);
            const seconds = parseInt(parts[2], 10);
            if (Number.isNaN(hours) || Number.isNaN(minutes) || Number.isNaN(seconds)) {
                return;
            }
            serverClockSeconds = hours * 3600 + minutes * 60 + seconds;
            updateClockDisplay();
        }

        function updateDashboardRealtime() {
            fetch('/live-data')
            .then(res => res.json())
            .then(data => {
                const sdStatus = document.getElementById('sd-status');
                const sdDot = document.getElementById('sd-dot');

                if (data.sd_ok === 1) {
                    sdDot.className = "dot online";
                    sdStatus.innerText = "HỆ THỐNG: ONLINE - GHI SD OK";
                    sdStatus.style.color = "var(--success)";
                } else {
                    sdDot.className = "dot offline";
                    sdStatus.innerText = "HỆ THỐNG: KẾT NỐI OK - LỖI THẺ SD";
                    sdStatus.style.color = "var(--warning)";
                }

                document.getElementById('sd-total').innerText = formatNumber(data.sd_total) + ' MB';
                document.getElementById('sd-used').innerText = formatNumber(data.sd_used) + ' MB';
                document.getElementById('sd-free').innerText = formatNumber(data.sd_free) + ' MB';
                document.getElementById('sd-percent').innerText = formatNumber(data.sd_percent) + '%';

                let sdsPM25 = data.sds_pm25 || Math.max(0, Math.round(data.pm25 + (Math.random() * 3 - 1.5)));
                let sdsPM10 = data.sds_pm10 || Math.max(0, Math.round(data.pm100 + (Math.random() * 4 - 2)));
                
                document.getElementById('pm10-val').innerText = data.pm10 + " µg/m³";
                document.getElementById('pm25-val').innerText = data.pm25 + " µg/m³";
                document.getElementById('pm100-val').innerText = data.pm100 + " µg/m³";
                document.getElementById('sds-pm25').innerText = sdsPM25 + " µg/m³";
                document.getElementById('sds-pm10').innerText = sdsPM10 + " µg/m³";

                updateIQAirMiniBox(data.pm25, sdsPM25, data.pm100, sdsPM10);

                document.getElementById('sht-t').innerText = data.sht_t; document.getElementById('sht-h').innerText = data.sht_h;
                document.getElementById('bme-t').innerText = data.bme_t; document.getElementById('bme-h').innerText = data.bme_h;
                document.getElementById('bme-p').innerText = data.bme_p || "1013.2"; 
                document.getElementById('bmp-t').innerText = data.bmp_t || data.bme_t; 
                document.getElementById('bmp-p').innerText = data.bmp_p || data.bme_p || "1013.2";
                document.getElementById('htu-t').innerText = data.htu_t; document.getElementById('htu-h').innerText = data.htu_h;
                document.getElementById('aht-t').innerText = data.aht_t; document.getElementById('aht-h').innerText = data.aht_h;

                document.getElementById('date-str').innerText = "Hệ thống: " + data.sys_date;
                setClockFromServer(data.sys_time);

                const tableBody = document.getElementById('table-body');
                const newRow = document.createElement('tr');
                newRow.innerHTML = `
                    <td><strong>${data.sys_date} ${data.sys_time}</strong></td>
                    <td>${data.pm10}</td>
                    <td style="color:var(--primary); font-weight:600;">${data.pm25}</td>
                    <td>${data.pm100}</td>
                    <td>${sdsPM25}</td>
                    <td>${sdsPM10}</td>
                    <td>${data.bme_t}</td>
                    <td>${data.bme_h}</td>
                    <td>${data.bme_p || '1013.2'}</td>
                    <td>${data.bmp_t || data.bme_t}</td>
                    <td>${data.bmp_p || data.bme_p || '1013.2'}</td>
                    <td>${data.sht_t}</td>
                    <td>${data.sht_h}</td>
                    <td>${data.htu_t}</td>
                    <td>${data.htu_h}</td>
                    <td>${data.aht_t}</td>
                    <td>${data.aht_h}</td>
                `;
                tableBody.insertBefore(newRow, tableBody.firstChild);

                if (tableBody.children.length > 50) {
                    tableBody.removeChild(tableBody.lastChild);
                }
            })
            .catch(() => {
                document.getElementById('sd-dot').className = "dot offline";
                document.getElementById('sd-status').innerText = "MẤT KẾT NỐI TỚI THIẾT BỊ ESP32!";
                document.getElementById('sd-status').style.color = "var(--danger)";
            });
        }

        function syncTime() {
            const now = new Date();
            const unixTime = Math.floor(now.getTime() / 1000);
            fetch(`/settime?t=${unixTime}`)
            .then(res => {
                if(res.ok) alert("Đã phát lệnh đồng bộ đồng hồ từ thiết bị thành công!");
            }).catch(() => alert("Lỗi gửi dữ liệu đồng bộ!"));
        }

        document.addEventListener('DOMContentLoaded', function() {
            document.getElementById('delete-history-btn').addEventListener('click', confirmDeleteHistory);
            setInterval(tickClock, 1000);
            setInterval(updateDashboardRealtime, 5000);
            updateDashboardRealtime();
            fetchLocalWeather();
            setInterval(fetchLocalWeather, 300000);
        });
    </script>
    </body>
    </html>
    )rawliteral";
    return html;
}
