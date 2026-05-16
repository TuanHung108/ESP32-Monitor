import os
import warnings
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from matplotlib.patches import Patch
from matplotlib.lines import Line2D
from scipy.stats import pearsonr
from itertools import combinations

warnings.filterwarnings("ignore")

# ── CẤU HÌNH ─────────────────────────────────────────────────────
BASE_DIR   = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.abspath(os.path.join(BASE_DIR, '..', '..'))
DATA_DIR   = os.path.join(PROJECT_DIR, 'day_measure')
OUTPUT_DIR = os.path.join(BASE_DIR, 'output_plots')
DPI        = 150

# Tên cột trong CSV
TEMP_COLS   = ['HTU_Nhiệt (°C)', 'SHT_Nhiệt (°C)', 'AHT_Nhiệt (°C)',
               'BME_Nhiệt (°C)', 'BMP_Nhiệt (°C)']
TEMP_LABELS = ['HTU21D', 'SHT31', 'AHT20', 'BME280', 'BMP280']
HUM_COLS    = ['HTU_Ẩm (%)', 'SHT_Ẩm (%)', 'AHT_Ẩm (%)', 'BME_Ẩm (%)']
HUM_LABELS  = ['HTU21D', 'SHT31', 'AHT20', 'BME280']
PM_COL      = 'PM2.5 (µg/m³)'
COLORS      = ['#3266ad', '#e24b4a', '#2eaa5e', '#f0982d', '#8e44ad']

# Phân chia giai đoạn
TUAN1_END = '2026-04-21'
TUAN2_S   = '2026-04-22'
TUAN2_END = '2026-04-28'
TUAN3_S   = '2026-04-29'

os.makedirs(OUTPUT_DIR, exist_ok=True)


# ── TẢI DỮ LIỆU ──────────────────────────────────────────────────
def load_data(data_dir):
    import glob
    files = sorted(glob.glob(os.path.join(data_dir, '*.csv')))
    if not files:
        raise FileNotFoundError(f"Không tìm thấy file CSV trong: {data_dir}")
    dfs = [pd.read_csv(f, parse_dates=['datetime']) for f in files]
    full = pd.concat(dfs, ignore_index=True).sort_values('datetime').reset_index(drop=True)
    full = full.set_index('datetime')
    print(f"Đã tải {len(files)} file | {len(full):,} điểm | "
          f"{full.index.min().date()} → {full.index.max().date()}")
    return full


# ── HELPER: tính correlation matrix ─────────────────────────────
def build_corr(full, cols):
    n = len(cols)
    mat = [[1.0 if i == j else np.nan for j in range(n)] for i in range(n)]
    for i in range(n):
        for j in range(n):
            if i != j:
                sub = full[[cols[i], cols[j]]].dropna()
                if len(sub) > 100:
                    r, _ = pearsonr(sub[cols[i]].values, sub[cols[j]].values)
                    mat[i][j] = float(r)
    return np.array(mat, dtype=float)


# ── HELPER: Bland-Altman panel ───────────────────────────────────
def ba_panel(ax, full, ca, cb, la, lb, color='#3266ad'):
    sub = full[[ca, cb]].dropna()
    m_ab = (sub[ca] + sub[cb]) / 2
    d_ab = sub[ca] - sub[cb]
    md, sd = float(d_ab.mean()), float(d_ab.std())
    idx_s = np.random.choice(len(sub), min(5000, len(sub)), replace=False)
    ax.scatter(m_ab.values[idx_s], d_ab.values[idx_s], s=1.5, alpha=0.2, color=color)
    ax.axhline(md,           color='#e24b4a', linewidth=1.8, label=f'Bias={md:.3f}')
    ax.axhline(md + 1.96*sd, color='#e24b4a', linewidth=1.2, linestyle='--',
               label=f'+1.96σ={md+1.96*sd:.3f}')
    ax.axhline(md - 1.96*sd, color='#e24b4a', linewidth=1.2, linestyle='--',
               label=f'−1.96σ={md-1.96*sd:.3f}')
    ax.axhline(0, color='gray', linewidth=0.8, linestyle=':')
    ax.set_title(f'{la} vs {lb}', fontsize=10)
    ax.legend(fontsize=7, loc='upper right')
    ax.yaxis.grid(True, alpha=0.25, linestyle='--')


# ── HÌNH 1: Boxplot nhiệt độ và độ ẩm ───────────────────────────
def fig1_boxplot(full):
    fig, axes = plt.subplots(1, 2, figsize=(13, 5))
    for ax, cols, labels, ylabel, title in [
        (axes[0], TEMP_COLS, TEMP_LABELS, 'Nhiệt độ (°C)', 'A. Phân phối nhiệt độ theo sensor'),
        (axes[1], HUM_COLS,  HUM_LABELS,  'Độ ẩm (%)',     'B. Phân phối độ ẩm theo sensor'),
    ]:
        data = [full[c].dropna().values for c in cols]
        bp = ax.boxplot(data, patch_artist=True, notch=False,
                        medianprops=dict(color='white', linewidth=2),
                        whiskerprops=dict(linewidth=1.2), capprops=dict(linewidth=1.2),
                        flierprops=dict(marker='.', markersize=2, alpha=0.2))
        for patch, col in zip(bp['boxes'], COLORS[:len(cols)]):
            patch.set_facecolor(col); patch.set_alpha(0.82)
        for i, (d, col) in enumerate(zip(data, COLORS[:len(cols)]), 1):
            ax.scatter(i, np.mean(d), marker='D', color='white',
                       edgecolors=col, s=45, zorder=5, linewidths=1.5)
        ax.set_xticks(range(1, len(labels)+1))
        ax.set_xticklabels(labels, fontsize=10)
        ax.set_ylabel(ylabel, fontsize=11)
        ax.set_title(title, fontsize=11, pad=8)
        ax.yaxis.grid(True, alpha=0.3, linestyle='--'); ax.set_axisbelow(True)
        ax.legend(handles=[Line2D([0],[0], marker='D', color='gray',
                                   markerfacecolor='white', label='Trung bình',
                                   linewidth=0)], fontsize=9)
    plt.tight_layout()
    path = f'{OUTPUT_DIR}/fig1_boxplot_temp_hum.png'
    plt.savefig(path, dpi=DPI, bbox_inches='tight'); plt.close()
    print(f'Đã lưu: {path}')


# ── HÌNH 2: Heatmap tương quan Pearson ──────────────────────────
def fig2_corr_heatmap(full):
    fig, axes = plt.subplots(1, 2, figsize=(14, 5.5))
    for ax, cols, labels, title, vmin in [
        (axes[0], TEMP_COLS, TEMP_LABELS, 'A. Tương quan nhiệt độ (Pearson r)', 0.85),
        (axes[1], HUM_COLS,  HUM_LABELS,  'B. Tương quan độ ẩm (Pearson r)',    0.0),
    ]:
        mat = build_corr(full, cols); n = len(cols)
        im = ax.imshow(mat, cmap='RdYlGn', vmin=vmin, vmax=1.0, aspect='auto')
        plt.colorbar(im, ax=ax, label='Pearson r', shrink=0.85)
        ax.set_xticks(range(n)); ax.set_yticks(range(n))
        ax.set_xticklabels(labels, rotation=30, ha='right', fontsize=9)
        ax.set_yticklabels(labels, fontsize=9)
        for i in range(n):
            for j in range(n):
                v = mat[i, j]
                if not np.isnan(v):
                    tc = 'white' if v < 0.6 else 'black'
                    ax.text(j, i, f'{v:.3f}', ha='center', va='center',
                            fontsize=9, color=tc, fontweight='bold')
        ax.set_title(title, fontsize=11, pad=10)
    plt.suptitle('Hình 2. Ma trận tương quan Pearson giữa các sensor', fontsize=12, y=1.02)
    plt.tight_layout()
    path = f'{OUTPUT_DIR}/fig3_corr_heatmap.png'
    plt.savefig(path, dpi=DPI, bbox_inches='tight'); plt.close()
    print(f'Đã lưu: {path}')


# ── HÌNH 3: Chuỗi thời gian (nhiệt + ẩm + PM2.5) ───────────────
def fig3_timeseries(full):
    fig, axes = plt.subplots(3, 1, figsize=(14, 11), sharex=True)
    # Panel A: nhiệt
    for c, lbl, col in zip(TEMP_COLS, TEMP_LABELS, COLORS):
        s = full[c].dropna(); rm = s.rolling(120, min_periods=30).mean()
        axes[0].plot(s.index, s.values, color=col, alpha=0.06, linewidth=0.3)
        axes[0].plot(rm.index, rm.values, color=col, linewidth=1.5, label=lbl)
    axes[0].set_ylabel('Nhiệt độ (°C)', fontsize=10)
    axes[0].set_title('A. Chuỗi thời gian nhiệt độ — toàn bộ sensor (rolling mean 10 phút)', fontsize=10)
    axes[0].legend(fontsize=9, loc='upper right', ncol=5)
    axes[0].yaxis.grid(True, alpha=0.25, linestyle='--'); axes[0].set_axisbelow(True)
    # Panel B: ẩm
    for c, lbl, col in zip(HUM_COLS, HUM_LABELS, COLORS):
        s = full[c].dropna(); rm = s.rolling(120, min_periods=30).mean()
        axes[1].plot(s.index, s.values, color=col, alpha=0.06, linewidth=0.3)
        axes[1].plot(rm.index, rm.values, color=col, linewidth=1.5, label=lbl)
    axes[1].set_ylabel('Độ ẩm (%)', fontsize=10)
    axes[1].set_title('B. Chuỗi thời gian độ ẩm — toàn bộ sensor (rolling mean 10 phút)', fontsize=10)
    axes[1].legend(fontsize=9, loc='upper right', ncol=4)
    axes[1].yaxis.grid(True, alpha=0.25, linestyle='--'); axes[1].set_axisbelow(True)
    # Panel C: PM2.5
    pm_s = full[PM_COL].dropna(); pm_rm = pm_s.rolling(120, min_periods=30).mean()
    axes[2].fill_between(pm_s.index, 0, pm_s.values, color='#3266ad', alpha=0.07)
    axes[2].plot(pm_s.index, pm_s.values, color='#3266ad', alpha=0.15, linewidth=0.3)
    axes[2].plot(pm_rm.index, pm_rm.values, color='#3266ad', linewidth=1.8, label='PM2.5')
    axes[2].axhline(75, color='#f0982d', linewidth=1.2, linestyle='--', label='QCVN 05 24h = 75 µg/m³')
    axes[2].set_ylabel('PM2.5 (µg/m³)', fontsize=10)
    axes[2].set_title('C. Chuỗi thời gian PM2.5 (rolling mean 10 phút)', fontsize=10)
    axes[2].legend(fontsize=9, loc='upper right')
    axes[2].yaxis.grid(True, alpha=0.25, linestyle='--'); axes[2].set_axisbelow(True)
    axes[2].xaxis.set_major_formatter(mdates.DateFormatter('%d/%m'))
    axes[2].xaxis.set_major_locator(mdates.DayLocator(interval=2))
    plt.setp(axes[2].xaxis.get_majorticklabels(), rotation=30, ha='right')
    d0 = full.index.min().strftime('%d/%m/%Y')
    d1 = full.index.max().strftime('%d/%m/%Y')
    plt.suptitle(f'Hình 3. Chuỗi thời gian đo lường — {d0} đến {d1}', fontsize=13, y=1.01)
    plt.tight_layout()
    path = f'{OUTPUT_DIR}/fig4_timeseries.png'
    plt.savefig(path, dpi=DPI, bbox_inches='tight'); plt.close()
    print(f'Đã lưu: {path}')


# ── HÌNH 4: Bland-Altman nhiệt độ (10 cặp) ──────────────────────
def fig4_ba_temp(full):
    pairs = list(combinations(range(5), 2))
    fig, axes = plt.subplots(3, 4, figsize=(22, 14))
    axes_flat = axes.flatten()
    for idx, (i, j) in enumerate(pairs):
        ax = axes_flat[idx]
        ba_panel(ax, full, TEMP_COLS[i], TEMP_COLS[j],
                 TEMP_LABELS[i], TEMP_LABELS[j], COLORS[i])
        ax.set_xlabel('Trung bình (°C)', fontsize=8)
        ax.set_ylabel(f'{TEMP_LABELS[i]}−{TEMP_LABELS[j]} (°C)', fontsize=8)
    for k in range(len(pairs), len(axes_flat)):
        axes_flat[k].set_visible(False)
    plt.suptitle('Hình 4. Bland-Altman Plot — Nhiệt độ (10 cặp sensor)', fontsize=14, y=1.01)
    plt.tight_layout()
    path = f'{OUTPUT_DIR}/fig5_bland_altman_temp.png'
    plt.savefig(path, dpi=DPI, bbox_inches='tight'); plt.close()
    print(f'Đã lưu: {path}')


# ── HÌNH 5: Bland-Altman độ ẩm (6 cặp) ─────────────────────────
def fig5_ba_hum(full):
    pairs = list(combinations(range(4), 2))
    fig, axes = plt.subplots(2, 3, figsize=(16, 10))
    axes_flat = axes.flatten()
    for idx, (i, j) in enumerate(pairs):
        ax = axes_flat[idx]
        ba_panel(ax, full, HUM_COLS[i], HUM_COLS[j],
                 HUM_LABELS[i], HUM_LABELS[j], '#2eaa5e')
        ax.set_xlabel('Trung bình (%)', fontsize=8)
        ax.set_ylabel(f'{HUM_LABELS[i]}−{HUM_LABELS[j]} (%)', fontsize=8)
    plt.suptitle('Hình 5. Bland-Altman Plot — Độ ẩm (6 cặp sensor)', fontsize=14, y=1.01)
    plt.tight_layout()
    path = f'{OUTPUT_DIR}/fig6_bland_altman_hum.png'
    plt.savefig(path, dpi=DPI, bbox_inches='tight'); plt.close()
    print(f'Đã lưu: {path}')


# ── HÌNH 6: CV theo ngày (PM2.5 + 5 sensor nhiệt) ───────────────
def fig6_daily_cv(full):
    pm    = full[PM_COL].dropna()
    dates = sorted(set(full.index.date))
    dates_str = [d.strftime('%d/%m') for d in dates]
    x = np.arange(len(dates))
    cv_pm = [float(pm[pm.index.date == d].dropna().pipe(
        lambda s: s.std()/s.mean()*100 if len(s) > 10 else np.nan)) for d in dates]

    fig, axes = plt.subplots(2, 1, figsize=(16, 8), sharex=True)
    ax = axes[0]
    colors_cv = ['#e24b4a' if (v and v > 50) else '#f0982d' if (v and v > 30)
                 else '#2eaa5e' for v in cv_pm]
    ax.bar(x, cv_pm, color=colors_cv, alpha=0.85, width=0.7)
    ax.axhline(30, color='#f0982d', linewidth=1.2, linestyle='--')
    ax.axhline(50, color='#e24b4a', linewidth=1.2, linestyle='--')
    for i, v in enumerate(cv_pm):
        if v: ax.text(i, v + 1.5, f'{v:.0f}%', ha='center', fontsize=6.5, color='#333')
    ax.set_ylabel('CV (%)', fontsize=10)
    ax.set_title('A. CV PM2.5 theo ngày', fontsize=11, pad=6)
    ax.legend(handles=[Patch(color='#2eaa5e', label='CV≤30%'),
                       Patch(color='#f0982d', label='30%<CV≤50%'),
                       Patch(color='#e24b4a', label='CV>50%')], fontsize=8)
    ax.yaxis.grid(True, alpha=0.3, linestyle='--'); ax.set_axisbelow(True)

    ax = axes[1]
    n_s = len(TEMP_COLS); w = 0.7 / n_s
    for i2, (c, lbl, col) in enumerate(zip(TEMP_COLS, TEMP_LABELS, COLORS)):
        cv_t = [float(full[c][full[c].index.date == d].dropna().pipe(
            lambda s: s.std()/s.mean()*100 if len(s) > 10 else np.nan)) for d in dates]
        ax.bar(x + (i2 - n_s/2 + 0.5)*w, cv_t, width=w*0.85,
               color=col, alpha=0.82, label=lbl)
    ax.set_ylabel('CV (%)', fontsize=10)
    ax.set_title('B. CV nhiệt độ theo ngày — tất cả 5 sensor', fontsize=11, pad=6)
    ax.set_xticks(x); ax.set_xticklabels(dates_str, rotation=30, ha='right', fontsize=7.5)
    ax.legend(fontsize=8, ncol=5)
    ax.yaxis.grid(True, alpha=0.3, linestyle='--'); ax.set_axisbelow(True)
    plt.tight_layout()
    path = f'{OUTPUT_DIR}/fig7_daily_cv.png'
    plt.savefig(path, dpi=DPI, bbox_inches='tight'); plt.close()
    print(f'Đã lưu: {path}')


# ── HÌNH 7: Inter-sensor difference (drift analysis) ────────────
def fig7_intersensor_diff(full):
    fig, axes = plt.subplots(2, 1, figsize=(16, 9), sharex=True)
    ax = axes[0]
    ax.axhline(0, color='gray', linewidth=1.2, linestyle='--', label='Ref: HTU21D')
    for c, lbl, col in zip(TEMP_COLS[1:], TEMP_LABELS[1:], COLORS[1:]):
        diff = (full[c] - full[TEMP_COLS[0]]).dropna()
        rm   = diff.rolling(720, min_periods=60).mean()
        ax.plot(rm.index, rm.values, color=col, linewidth=1.5, label=f'{lbl}−HTU21D')
    ax.set_ylabel('Hiệu số nhiệt (°C)', fontsize=10)
    ax.set_title('A. Hiệu số nhiệt độ so với HTU21D (rolling 1 giờ)', fontsize=10, pad=6)
    ax.legend(fontsize=9, ncol=2); ax.yaxis.grid(True, alpha=0.25, linestyle='--')
    ax.set_axisbelow(True)

    ax = axes[1]
    ax.axhline(0, color='gray', linewidth=1.2, linestyle='--', label='Ref: HTU21D')
    for c, lbl, col in zip(HUM_COLS[1:], HUM_LABELS[1:], COLORS[1:]):
        diff = (full[c] - full[HUM_COLS[0]]).dropna()
        rm   = diff.rolling(720, min_periods=60).mean()
        ax.plot(rm.index, rm.values, color=col, linewidth=1.5, label=f'{lbl}−HTU21D')
    ax.set_ylabel('Hiệu số độ ẩm (%)', fontsize=10)
    ax.set_title('B. Hiệu số độ ẩm so với HTU21D (rolling 1 giờ)', fontsize=10, pad=6)
    ax.legend(fontsize=9, ncol=2); ax.yaxis.grid(True, alpha=0.25, linestyle='--')
    ax.set_axisbelow(True)
    ax.xaxis.set_major_formatter(mdates.DateFormatter('%d/%m'))
    ax.xaxis.set_major_locator(mdates.DayLocator(interval=2))
    plt.setp(ax.xaxis.get_majorticklabels(), rotation=30, ha='right')
    d0 = full.index.min().strftime('%d/%m/%Y')
    d1 = full.index.max().strftime('%d/%m/%Y')
    plt.suptitle(f'Hình 7. Hiệu số liên sensor theo thời gian — phân tích drift ({d0}–{d1})',
                 fontsize=12, y=1.01)
    plt.tight_layout()
    path = f'{OUTPUT_DIR}/fig8_intersensor_diff.png'
    plt.savefig(path, dpi=DPI, bbox_inches='tight'); plt.close()
    print(f'Đã lưu: {path}')


# ── HÌNH 8: So sánh 3 giai đoạn (PM2.5 + 5 sensor nhiệt) ───────
def fig8_weekly_compare(full):
    pm    = full[PM_COL].dropna()
    dates = sorted(set(full.index.date))
    t1_end = pd.Timestamp(TUAN1_END).date()
    t2_s   = pd.Timestamp(TUAN2_S).date()
    t2_end = pd.Timestamp(TUAN2_END).date()
    t3_s   = pd.Timestamp(TUAN3_S).date()

    def pm_slice(s, e):
        mask = np.array([(d >= s and d <= e) for d in pm.index.date])
        return pm.values[mask]

    week1 = pm_slice(dates[0], t1_end)
    week2 = pm_slice(t2_s, t2_end)
    week3 = pm_slice(t3_s, dates[-1])

    fig, axes = plt.subplots(1, 2, figsize=(14, 5))
    ax = axes[0]
    bp = ax.boxplot([week1, week2, week3], patch_artist=True,
                    medianprops=dict(color='white', linewidth=2),
                    flierprops=dict(marker='.', markersize=2, alpha=0.2))
    for patch, col in zip(bp['boxes'], COLORS[:3]):
        patch.set_facecolor(col); patch.set_alpha(0.8)
    for i2, (d, col) in enumerate(zip([week1, week2, week3], COLORS[:3]), 1):
        ax.scatter(i2, d.mean(), marker='D', s=50, color='white',
                   edgecolors=col, zorder=5, linewidths=1.5)
    ax.set_xticks([1, 2, 3])
    ax.set_xticklabels([
        f'Tuần 1\n(15–21/04)\nn={len(week1):,}',
        f'Tuần 2\n(22–28/04)\nn={len(week2):,}',
        f'Tuần 3+\n(29/04–06/05)\nn={len(week3):,}',
    ])
    ax.set_ylabel('PM2.5 (µg/m³)', fontsize=10)
    ax.set_title('A. PM2.5: Tuần 1 vs Tuần 2 vs Tuần 3+', fontsize=11, pad=8)
    ax.yaxis.grid(True, alpha=0.3, linestyle='--'); ax.set_axisbelow(True)

    ax = axes[1]
    w_bar = 0.14
    periods = [(dates[0], t1_end), (t2_s, t2_end), (t3_s, dates[-1])]
    xlabels = ['Tuần 1\n(15–21/04)', 'Tuần 2\n(22–28/04)', 'Tuần 3+\n(29/04–06/05)']
    x3 = np.array([1, 2, 3])
    for i2, (c, lbl, col) in enumerate(zip(TEMP_COLS, TEMP_LABELS, COLORS)):
        means, errs = [], []
        for s_d, e_d in periods:
            vals = full[c][[d >= s_d and d <= e_d for d in full[c].index.date]].dropna()
            means.append(float(vals.mean())); errs.append(float(vals.std()))
        offset = (i2 - len(TEMP_COLS)/2 + 0.5) * w_bar
        ax.bar(x3 + offset, means, width=w_bar*0.85, color=col, alpha=0.85, label=lbl)
        ax.errorbar(x3 + offset, means, yerr=errs, fmt='none',
                    color='black', capsize=2, linewidth=0.8)
    ax.set_xticks(x3); ax.set_xticklabels(xlabels)
    ax.set_ylabel('Nhiệt độ trung bình (°C)', fontsize=10)
    ax.set_title('B. Nhiệt độ trung bình — 5 sensor theo 3 giai đoạn', fontsize=11, pad=8)
    ax.legend(fontsize=8, ncol=5)
    ax.yaxis.grid(True, alpha=0.3, linestyle='--'); ax.set_axisbelow(True)
    ax.set_ylim(26, 32)
    plt.suptitle('Hình 8. So sánh PM2.5 và nhiệt độ theo 3 giai đoạn', fontsize=12, y=1.02)
    plt.tight_layout()
    path = f'{OUTPUT_DIR}/fig12_weekly_compare.png'
    plt.savefig(path, dpi=DPI, bbox_inches='tight'); plt.close()
    print(f'Đã lưu: {path}')


# ── HÌNH 9: Scatter PM2.5 vs Độ ẩm và Nhiệt độ ─────────────────
def fig9_pm_vs_env(full):
    from scipy.stats import linregress
    pm   = full[PM_COL].dropna()
    sub  = full[[PM_COL, 'HTU_Ẩm (%)', 'HTU_Nhiệt (°C)']].dropna()
    idx_s = np.random.choice(len(sub), min(8000, len(sub)), replace=False)
    sub_s = sub.iloc[idx_s]

    fig, axes = plt.subplots(1, 2, figsize=(13, 5))
    ax = axes[0]
    sc = ax.scatter(sub_s['HTU_Ẩm (%)'], sub_s[PM_COL],
                    c=sub_s['HTU_Nhiệt (°C)'], cmap='coolwarm',
                    s=4, alpha=0.4, vmin=26, vmax=33)
    plt.colorbar(sc, ax=ax, label='Nhiệt độ (°C)', shrink=0.85)
    sl, ic, r, *_ = linregress(sub['HTU_Ẩm (%)'].values, sub[PM_COL].values)
    xr = np.linspace(sub['HTU_Ẩm (%)'].min(), sub['HTU_Ẩm (%)'].max(), 200)
    ax.plot(xr, sl*xr+ic, color='black', linewidth=2, linestyle='--',
            label=f'y={sl:.2f}x{ic:+.1f}\nr={r:.3f}')
    ax.set_xlabel('Độ ẩm HTU21D (%)', fontsize=11)
    ax.set_ylabel('PM2.5 (µg/m³)', fontsize=11)
    ax.set_title('A. PM2.5 vs Độ ẩm\n(màu = nhiệt độ)', fontsize=11, pad=8)
    ax.legend(fontsize=9); ax.yaxis.grid(True, alpha=0.25, linestyle='--')
    ax.set_axisbelow(True)

    ax = axes[1]
    sc2 = ax.scatter(sub_s['HTU_Nhiệt (°C)'], sub_s[PM_COL],
                     c=sub_s['HTU_Ẩm (%)'], cmap='Blues',
                     s=4, alpha=0.4, vmin=40, vmax=90)
    plt.colorbar(sc2, ax=ax, label='Độ ẩm (%)', shrink=0.85)
    sl2, ic2, r2, *_ = linregress(sub['HTU_Nhiệt (°C)'].values, sub[PM_COL].values)
    xr2 = np.linspace(sub['HTU_Nhiệt (°C)'].min(), sub['HTU_Nhiệt (°C)'].max(), 200)
    ax.plot(xr2, sl2*xr2+ic2, color='black', linewidth=2, linestyle='--',
            label=f'y={sl2:.2f}x{ic2:+.1f}\nr={r2:.3f}')
    ax.set_xlabel('Nhiệt độ HTU21D (°C)', fontsize=11)
    ax.set_ylabel('PM2.5 (µg/m³)', fontsize=11)
    ax.set_title('B. PM2.5 vs Nhiệt độ\n(màu = độ ẩm)', fontsize=11, pad=8)
    ax.legend(fontsize=9); ax.yaxis.grid(True, alpha=0.25, linestyle='--')
    ax.set_axisbelow(True)
    plt.suptitle('Hình 9. Tương quan PM2.5 với điều kiện môi trường', fontsize=12, y=1.02)
    plt.tight_layout()
    path = f'{OUTPUT_DIR}/fig9_pm_vs_env.png'
    plt.savefig(path, dpi=DPI, bbox_inches='tight'); plt.close()
    print(f'Đã lưu: {path}')


# ── HÌNH 10: Heatmap PM2.5 theo giờ × ngày ──────────────────────
def fig10_heatmap_hourday(full):
    dates     = sorted(set(full.index.date))
    date_order = [d.strftime('%d/%m') for d in dates]
    date_str_arr = np.array([d.strftime('%d/%m') for d in full.index.date])
    hour_arr     = np.array(full.index.hour)
    pm_arr       = full[PM_COL].values

    rows = []
    for h in range(24):
        row = []
        for ds in date_order:
            mask = (hour_arr == h) & (date_str_arr == ds)
            vals = pm_arr[mask]; vals = vals[~np.isnan(vals)]
            row.append(float(vals.mean()) if len(vals) > 0 else np.nan)
        rows.append(row)
    pivot = np.array(rows, dtype=float)

    fig, ax = plt.subplots(figsize=(18, 5))
    im = ax.imshow(pivot, cmap='YlOrRd', aspect='auto',
                   interpolation='nearest', vmin=0, vmax=150)
    plt.colorbar(im, ax=ax, label='PM2.5 trung bình (µg/m³)', shrink=0.9)
    ax.set_xticks(range(len(date_order)))
    ax.set_xticklabels(date_order, rotation=45, ha='right', fontsize=8)
    ax.set_yticks(range(0, 24, 2))
    ax.set_yticklabels([f'{h:02d}:00' for h in range(0, 24, 2)], fontsize=8)
    ax.set_xlabel('Ngày', fontsize=11); ax.set_ylabel('Giờ trong ngày', fontsize=11)
    d0 = dates[0].strftime('%d/%m/%Y'); d1 = dates[-1].strftime('%d/%m/%Y')
    ax.set_title(f'Hình 10. Heatmap PM2.5 trung bình theo giờ và ngày ({d0}–{d1})',
                 fontsize=11, pad=8)
    plt.tight_layout()
    path = f'{OUTPUT_DIR}/fig10_pm_heatmap_hourday.png'
    plt.savefig(path, dpi=DPI, bbox_inches='tight'); plt.close()
    print(f'Đã lưu: {path}')


# ── HÌNH 11: Scatter PM2.5 vs Độ ẩm theo giờ ───────────────────
def fig11_pm_rh_scatter_hourly(full):
    sub2 = full[[PM_COL, 'HTU_Ẩm (%)', 'HTU_Nhiệt (°C)']].dropna().copy()
    sub2['hour'] = sub2.index.hour
    idx_s = np.random.choice(len(sub2), min(12000, len(sub2)), replace=False)
    sub_plot = sub2.iloc[idx_s]

    fig, ax = plt.subplots(figsize=(9, 6))
    sc3 = ax.scatter(sub_plot['HTU_Ẩm (%)'], sub_plot[PM_COL],
                     c=sub_plot['hour'], cmap='twilight',
                     s=4, alpha=0.35, vmin=0, vmax=23)
    cb = plt.colorbar(sc3, ax=ax, label='Giờ trong ngày', shrink=0.9)
    cb.set_ticks([0, 6, 12, 18, 23])
    cb.set_ticklabels(['00:00','06:00','12:00','18:00','23:00'])

    bins = [40, 50, 60, 65, 70, 75, 80, 85, 90]
    bin_means, bin_centers = [], []
    for lo, hi in zip(bins[:-1], bins[1:]):
        mask = (sub2['HTU_Ẩm (%)'] >= lo) & (sub2['HTU_Ẩm (%)'] < hi)
        if mask.sum() > 50:
            bin_means.append(float(sub2.loc[mask, PM_COL].mean()))
            bin_centers.append((lo + hi) / 2)
    ax.plot(bin_centers, bin_means, 'ko-', linewidth=2, markersize=7,
            label='Mean PM2.5 theo khoảng RH', zorder=5)
    ax.set_xlabel('Độ ẩm HTU21D (%)', fontsize=11)
    ax.set_ylabel('PM2.5 (µg/m³)', fontsize=11)
    ax.set_title('Hình 11. Scatter PM2.5 vs Độ ẩm\n(màu = giờ; đường đen = trung bình theo khoảng RH)',
                 fontsize=11, pad=8)
    ax.legend(fontsize=9); ax.yaxis.grid(True, alpha=0.25, linestyle='--')
    ax.set_axisbelow(True)
    plt.tight_layout()
    path = f'{OUTPUT_DIR}/fig13_pm_rh_scatter.png'
    plt.savefig(path, dpi=DPI, bbox_inches='tight'); plt.close()
    print(f'Đã lưu: {path}')


# ── MAIN ─────────────────────────────────────────────────────────
if __name__ == '__main__':
    print('=' * 55)
    print('  VẼ BIỂU ĐỒ PHÂN TÍCH CẢM BIẾN MÔI TRƯỜNG')
    print('=' * 55)

    full = load_data(DATA_DIR)

    print('\n[1/11] Boxplot nhiệt độ và độ ẩm...')
    fig1_boxplot(full)

    print('[2/11] Heatmap tương quan Pearson...')
    fig2_corr_heatmap(full)

    print('[3/11] Chuỗi thời gian...')
    fig3_timeseries(full)

    print('[4/11] Bland-Altman — nhiệt độ (10 cặp)...')
    fig4_ba_temp(full)

    print('[5/11] Bland-Altman — độ ẩm (6 cặp)...')
    fig5_ba_hum(full)

    print('[6/11] CV theo ngày...')
    fig6_daily_cv(full)

    print('[7/11] Inter-sensor difference (drift)...')
    fig7_intersensor_diff(full)

    print('[8/11] So sánh 3 giai đoạn...')
    fig8_weekly_compare(full)

    print('[9/11] Scatter PM2.5 vs môi trường...')
    fig9_pm_vs_env(full)

    print('[10/11] Heatmap PM2.5 theo giờ × ngày...')
    fig10_heatmap_hourday(full)

    print('[11/11] Scatter PM2.5 vs Độ ẩm theo giờ...')
    fig11_pm_rh_scatter_hourly(full)

    print('\n' + '=' * 55)
    print(f'  Xong! Tất cả biểu đồ đã lưu vào: {OUTPUT_DIR}')
    print('=' * 55)
