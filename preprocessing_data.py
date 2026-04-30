import pandas as pd
import os
from glob import glob

# ================= CONFIG =================
INPUT_PATTERN = "C:/Users/PC/Downloads/VXL/Code/Project/Data/datalog*.csv"
OUTPUT_DIR = "C:/Users/PC/Downloads/VXL/Code/Project/day_measure"
SAMPLE_INTERVAL = "5s"

# Ngưỡng xử lý
MAX_VALID_STEP = 60        # tối đa 60s coi là hợp lệ
INTERPOLATE_LIMIT = 3      # nội suy tối đa 3 điểm (≈15s)
GAP_THRESHOLD = 20         # >20s coi là mất dữ liệu thật

os.makedirs(OUTPUT_DIR, exist_ok=True)

# ================= STEP 1: LOAD FILE =================
files = sorted(glob(INPUT_PATTERN))
print("Files found:", files)

df_list = []
for file in files:
    print(f"Reading {file}")
    df = pd.read_csv(file, sep=';')
    df_list.append(df)

df = pd.concat(df_list, ignore_index=True)

# ================= STEP 2: CREATE DATETIME =================
df['datetime'] = pd.to_datetime(
    df['Ngày đo'] + ' ' + df['Giờ đo'],
    format='%Y/%m/%d %H:%M:%S',
    errors='coerce'
)

df = df.dropna(subset=['datetime'])

# ================= STEP 3: SORT & CLEAN =================
df = df.sort_values('datetime')

# Xóa trùng timestamp
df = df.drop_duplicates(subset='datetime', keep='first')

# ================= STEP 4: REMOVE TIME ERROR =================
dt_diff = df['datetime'].diff().dt.total_seconds()

df = df[
    (dt_diff.isna()) | 
    ((dt_diff >= 0) & (dt_diff < MAX_VALID_STEP))
]

# ================= STEP 5: SET INDEX =================
df = df.set_index('datetime')
df = df.select_dtypes(include='number')

# ================= STEP 6: SPLIT BY DAY =================
for day, df_day in df.groupby(df.index.date):

    print(f"\nProcessing {day}...")

    df_day = df_day.sort_index()

    # ===== THỐNG KÊ RAW =====
    raw_count = len(df_day)

    # ===== STEP 7: RESAMPLE GIỮ OFFSET =====
    start_time = df_day.index[0]

    df_resampled = df_day.resample(
        SAMPLE_INTERVAL,
        origin=start_time   # 🔥 giữ pha thời gian
    ).mean()

    # ===== STEP 8: XỬ LÝ GAP =====
    dt_gap = df_resampled.index.to_series().diff().dt.total_seconds()

    # Đánh dấu gap lớn
    large_gap_mask = dt_gap > GAP_THRESHOLD

    # Nội suy gap nhỏ
    df_resampled_interp = df_resampled.interpolate(limit=INTERPOLATE_LIMIT)

    # Khôi phục lại gap lớn = NaN
    df_resampled_interp[large_gap_mask] = None

    # ===== STEP 9: THỐNG KÊ =====
    total_points = len(df_resampled_interp)
    missing_points = df_resampled_interp.isna().any(axis=1).sum()

    print(f"Raw samples: {raw_count}")
    print(f"After resample: {total_points}")
    print(f"Missing points: {missing_points}")
    print(f"Missing ratio: {missing_points/total_points:.2%}")

    # ===== STEP 10: SAVE =====
    output_file = os.path.join(OUTPUT_DIR, f"{day}.csv")
    df_resampled_interp.to_csv(output_file)

print("\nDone!")