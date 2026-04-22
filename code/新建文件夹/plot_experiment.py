import pandas as pd
import matplotlib.pyplot as plt

# ====== 修改这里：你的 summary 文件名 ======
CSV_FILE = "./compact_exp_summary.csv"

# ====== 读取数据 ======
df = pd.read_csv(CSV_FILE)

grow = df[df["phase"] == "grow"].copy()
shrink = df[df["phase"] == "shrink"].copy()

# 为了画图稳定，先排序
grow = grow.sort_values(["targetN", "r"])
shrink = shrink.sort_values(["targetN", "r"])

# --------------------------------------------------
# 图1：固定 N，比 r 对平均字节利用率的影响
# --------------------------------------------------
plt.figure()
for N in sorted(grow["targetN"].unique()):
    sub = grow[grow["targetN"] == N].sort_values("r")
    plt.plot(sub["r"], sub["avg_byte_util"], marker="o", label=f"N={N}")

plt.xlabel("r")
plt.ylabel("Average Byte Utilization (%)")
plt.title("Grow: r vs Average Byte Utilization")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("fig1_grow_r_vs_avg_byte_util.png", dpi=200)
plt.close()

# --------------------------------------------------
# 图2：固定 N，比 r 对最大瞬时额外空间的影响
# --------------------------------------------------
plt.figure()
for N in sorted(grow["targetN"].unique()):
    sub = grow[grow["targetN"] == N].sort_values("r")
    plt.plot(sub["r"], sub["max_transient_extra_bytes"], marker="o", label=f"N={N}")

plt.xlabel("r")
plt.ylabel("Max Transient Extra Bytes")
plt.title("Grow: r vs Max Transient Extra Space")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("fig2_grow_r_vs_max_transient.png", dpi=200)
plt.close()

# --------------------------------------------------
# 图3：固定 r，比 N 对平均字节利用率的影响
# --------------------------------------------------
plt.figure()
for r in sorted(grow["r"].unique()):
    sub = grow[grow["r"] == r].sort_values("targetN")
    plt.plot(sub["targetN"], sub["avg_byte_util"], marker="o", label=f"r={r}")

plt.xscale("log")
plt.xlabel("N")
plt.ylabel("Average Byte Utilization (%)")
plt.title("Grow: N vs Average Byte Utilization")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("fig3_grow_N_vs_avg_byte_util.png", dpi=200)
plt.close()

# --------------------------------------------------
# 图4：固定 r，比 N 对 B 的影响
# --------------------------------------------------
plt.figure()
for r in sorted(grow["r"].unique()):
    sub = grow[grow["r"] == r].sort_values("targetN")
    plt.plot(sub["targetN"], sub["B_at_targetN"], marker="o", label=f"r={r}")

plt.xscale("log")
plt.yscale("log")
plt.xlabel("N")
plt.ylabel("B at target N")
plt.title("Grow: N vs B")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("fig4_grow_N_vs_B.png", dpi=200)
plt.close()

# --------------------------------------------------
# 图5：固定 r，比 N 对 B / N^(1/r) 的影响
# --------------------------------------------------
plt.figure()
for r in sorted(grow["r"].unique()):
    sub = grow[grow["r"] == r].sort_values("targetN")
    plt.plot(sub["targetN"], sub["B_over_N_1_over_r_at_targetN"], marker="o", label=f"r={r}")

plt.xscale("log")
plt.xlabel("N")
plt.ylabel("B / N^(1/r)")
plt.title("Grow: N vs B / N^(1/r)")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("fig5_grow_N_vs_B_ratio.png", dpi=200)
plt.close()

# --------------------------------------------------
# 图6：固定 r，比 N 对指针开销占比的影响
# --------------------------------------------------
plt.figure()
for r in sorted(grow["r"].unique()):
    sub = grow[grow["r"] == r].sort_values("targetN")
    ratio = sub["pointer_bytes_at_targetN"] / sub["resident_bytes_at_targetN"]
    plt.plot(sub["targetN"], ratio, marker="o", label=f"r={r}")

plt.xscale("log")
plt.xlabel("N")
plt.ylabel("Pointer Bytes / Resident Bytes")
plt.title("Grow: N vs Pointer Overhead Ratio")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("fig6_grow_N_vs_pointer_ratio.png", dpi=200)
plt.close()

# --------------------------------------------------
# 图7：grow / shrink 平均字节利用率对比
# 每个 r 画两条线：grow 与 shrink
# --------------------------------------------------
# 这里选最大的 N 做对比，更直观
maxN = grow["targetN"].max()

g_sub = grow[grow["targetN"] == maxN].sort_values("r")
s_sub = shrink[shrink["targetN"] == maxN].sort_values("r")

plt.figure()
plt.plot(g_sub["r"], g_sub["avg_byte_util"], marker="o", label=f"grow, N={maxN}")
plt.plot(s_sub["r"], s_sub["avg_byte_util"], marker="o", label=f"shrink, N={maxN}")

plt.xlabel("r")
plt.ylabel("Average Byte Utilization (%)")
plt.title("Grow vs Shrink: Average Byte Utilization")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("fig7_grow_vs_shrink_avg_byte_util.png", dpi=200)
plt.close()

# --------------------------------------------------
# 图8：grow / shrink 最大瞬时额外空间对比
# --------------------------------------------------
plt.figure()
plt.plot(g_sub["r"], g_sub["max_transient_extra_bytes"], marker="o", label=f"grow, N={maxN}")
plt.plot(s_sub["r"], s_sub["max_transient_extra_bytes"], marker="o", label=f"shrink, N={maxN}")

plt.xlabel("r")
plt.ylabel("Max Transient Extra Bytes")
plt.title("Grow vs Shrink: Max Transient Extra Space")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("fig8_grow_vs_shrink_max_transient.png", dpi=200)
plt.close()

print("All summary-only figures generated successfully.")