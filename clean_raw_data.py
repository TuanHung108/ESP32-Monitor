import pandas as pd
import numpy as np
import os
import re
from glob import glob

# ================= CONFIG =================
RAW_INPUT_PATTERN = "C:/Users/PC/Downloads/VXL/Code/Project/Data/datalog14.csv"
CLEAN_OUTPUT_DIR = "C:/Users/PC/Downloads/VXL/Code/Project/Data_Cleaned"
os.makedirs(CLEAN_OUTPUT_DIR, exist_ok=True)


def clean_sensor_data(df):
    temp_cols = [
        'BME_Nhiệt (°C)', 'BMP_Nhiệt (°C)', 'HTU_Nhiệt (°C)', 'SHT_Nhiệt (°C)', 'AHT_Nhiệt (°C)'
    ]

    humi_cols = [
        'BME_Ẩm (%)', 'HTU_Ẩm (%)', 'SHT_Ẩm (%)', 'AHT_Ẩm (%)'
    ]

    pres_cols = [
        'BME_Áp suất (hPa)', 'BMP_Áp suất (hPa)'
    ]

    # ================= PM COLUMNS AUTO DETECTION =================
    pms_cols = []
    sds_cols = []

    if 'PM1.0 (µg/m³)' in df.columns:
        pms_cols = ['PM1.0 (µg/m³)', 'PM2.5 (µg/m³)', 'PM10 (µg/m³)']
    elif 'PMS_PM1.0 (µg/m³)' in df.columns:
        pms_cols = ['PMS_PM1.0 (µg/m³)', 'PMS_PM2.5 (µg/m³)', 'PMS_PM10 (µg/m³)']
    
    if 'SDS_PM2.5 (µg/m³)' in df.columns:
        sds_cols = ['SDS_PM2.5 (µg/m³)', 'SDS_PM10 (µg/m³)']

    # Đảm bảo tất cả các cột dữ liệu số đều ở đúng kiểu dữ liệu float để tính toán
    all_numeric_cols = temp_cols + humi_cols + pres_cols + pms_cols + sds_cols
    for col in all_numeric_cols:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors='coerce')

    total_fixed = 0

    # ================= 1. REPLACE ERROR VALUES =================
    error_values = [182.47, 100.00, -131.08, -138.22, 179.30]

    for val in error_values:
        active_cols = [c for c in all_numeric_cols if c in df.columns]
        if active_cols:
            count = (df[active_cols] == val).sum().sum()
            total_fixed += count
            df[active_cols] = df[active_cols].replace(val, np.nan)

    # ================= 2. PHYSICAL LIMIT CHECK =================
    for col in temp_cols:
        if col in df.columns:
            mask = (df[col] > 60) | (df[col] < 0)
            total_fixed += mask.sum()
            df.loc[mask, col] = np.nan

    for col in humi_cols:
        if col in df.columns:
            mask = (df[col] > 100) | (df[col] < 0)
            total_fixed += mask.sum()
            df.loc[mask, col] = np.nan

    for col in pres_cols:
        if col in df.columns:
            mask = (df[col] < 950) | (df[col] > 1050)
            total_fixed += mask.sum()
            df.loc[mask, col] = np.nan

    for col in pms_cols:
        if col in df.columns:
            mask = (df[col] < 0) | (df[col] > 2000)
            total_fixed += mask.sum()
            df.loc[mask, col] = np.nan

    for col in sds_cols:
        if col in df.columns:
            mask = (df[col] < 0) | (df[col] > 1000)
            total_fixed += mask.sum()
            df.loc[mask, col] = np.nan

    # ================= 3. SMART FILL =================
    def smart_fill_with_bias(target_col, source_cols):
        if target_col not in df.columns:
            return

        if df[target_col].isnull().any():
            for src in source_cols:
                if src not in df.columns or target_col == src:
                    continue
                if not df[src].notnull().any():
                    continue

                mask = (df[target_col].notnull() & df[src].notnull())

                if mask.sum() > 50:
                    actual_bias = (df[target_col][mask] - df[src][mask]).mean()
                    df[target_col] = df[target_col].fillna(df[src] + actual_bias)
                else:
                    df[target_col] = df[target_col].fillna(df[src])

    for col in temp_cols: smart_fill_with_bias(col, temp_cols)
    for col in humi_cols: smart_fill_with_bias(col, humi_cols)
    for col in pres_cols: smart_fill_with_bias(col, pres_cols)
        
    if len(pms_cols) > 0 and len(sds_cols) > 0:
        pms_pm25 = next((c for c in pms_cols if 'PM2.5' in c), None)
        sds_pm25 = next((c for c in sds_cols if 'PM2.5' in c), None)
        if pms_pm25 and sds_pm25:
            smart_fill_with_bias(pms_pm25, [sds_pm25])
            smart_fill_with_bias(sds_pm25, [pms_pm25])
            
        pms_pm10 = next((c for c in pms_cols if 'PM10' in c), None)
        sds_pm10 = next((c for c in sds_cols if 'PM10' in c), None)
        if pms_pm10 and sds_pm10:
            smart_fill_with_bias(pms_pm10, [sds_pm10])
            smart_fill_with_bias(sds_pm10, [pms_pm10])

    # ================= 4. INTERPOLATE =================
    before_nan = df.isna().sum().sum()
    df = df.interpolate(method='linear', limit=12)
    df = df.ffill().bfill()
    after_nan = df.isna().sum().sum()
    total_fixed += before_nan - after_nan

    # ================= 5. ROUND & CAST =================
    for col in temp_cols + humi_cols + pres_cols:
        if col in df.columns:
            df[col] = df[col].round(2)

    for col in pms_cols:
        if col in df.columns:
            df[col] = df[col].fillna(0).round(0).astype(int)

    for col in sds_cols:
        if col in df.columns:
            df[col] = df[col].fillna(0.0).round(1)

    print(f"Đã sửa và bù nhiễu khoảng {int(total_fixed)} ô giá trị.")
    return df


# ================= EXECUTE (SỬA LỖI CHUỖI THÔ TRƯỚC KHI ĐỌC) =================

files = sorted(glob(RAW_INPUT_PATTERN))

for file in files:
    filename = os.path.basename(file)
    print(f"\nĐang xử lý cấu trúc thô: {filename}...")

    try:
        # Bước 1: Đọc file dưới dạng văn bản thuần túy (text) để sửa lỗi dính dòng và thừa chấm phẩy
        with open(file, 'r', encoding='utf-8') as f:
            lines = f.readlines()

        cleaned_lines = []
        for i, line in enumerate(lines):
            line_str = line.strip()
            if not line_str:
                continue
            
            # Khắc phục lỗi hàng 1 bị dính header: Tìm vị trí năm "2026/" dính sau ký tự đóng ngoặc ")"
            if i == 0 and ")2026/" in line_str:
                line_str = line_str.replace(")2026/", ")\n2026/")
                # Tách thành 2 dòng riêng biệt để xử lý tiếp
                sub_lines = line_str.split('\n')
            else:
                sub_lines = [line_str]

            for sub_line in sub_lines:
                # Xóa sạch tất cả các dấu chấm phẩy lặp đi lặp lại vô nghĩa ở cuối dòng
                sub_line = re.sub(r';+$', '', sub_line)
                
                # Sửa lỗi số thực dị dạng có 2 dấu chấm (Ví dụ: 0.0.1 -> 0.0)
                sub_line = re.sub(r'(\d+\.\d+)\.\d+', r'\1', sub_line)
                
                if sub_line:
                    cleaned_lines.append(sub_line + '\n')

        # Bước 2: Khởi tạo DataFrame từ danh sách các dòng văn bản đã được lọc sạch
        from io import StringIO
        csv_data = StringIO("".join(cleaned_lines))
        
        df_raw = pd.read_csv(csv_data, sep=';', encoding='utf-8')
        print(f"Số dòng thực tế nhận diện: {len(df_raw)}")
        
        # Loại bỏ triệt để các cột Unnamed phát sinh ngoài ý muốn nếu có
        df_raw = df_raw.loc[:, ~df_raw.columns.str.contains('^Unnamed')]

        # Bước 3: Đưa vào hàm xử lý toán học lý hóa môi trường
        df_cleaned = clean_sensor_data(df_raw)

        # Bước 4: Xuất file sạch
        output_path = os.path.join(CLEAN_OUTPUT_DIR, filename)
        df_cleaned.to_csv(output_path, sep=';', index=False, encoding='utf-8')
        print(f"Đã lưu file hoàn toàn sạch tại: {output_path}")

    except Exception as e:
        print(f"Lỗi hệ thống khi xử lý file {filename}: {e}")

print("\n--- HOÀN THÀNH HỆ THỐNG XỬ LÝ ---")