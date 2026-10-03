"""Compile + jalankan simulasi.cpp (kode ArahMPU6050 asli + MPU6050 tiruan), lalu gambar grafik ke ../gambar/."""
import glob, os, subprocess, sys, tempfile

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

plt.rcParams.update({
    "figure.figsize": (8, 3.6), "figure.dpi": 100, "savefig.bbox": "tight", "savefig.pad_inches": 0.15,
    "figure.facecolor": "white", "axes.facecolor": "white", "savefig.facecolor": "white",
    "font.size": 10, "axes.titlesize": 11, "axes.titleweight": "bold", "axes.titlelocation": "left",
    "axes.spines.top": False, "axes.spines.right": False, "axes.edgecolor": "#9ca3af",
    "axes.grid": True, "grid.color": "#e5e7eb", "grid.linewidth": 0.8,
    "legend.frameon": False, "svg.fonttype": "path", "svg.hashsalt": "nothinx",
    "lines.linewidth": 1.8,
})
WARNA = {"utama": "#2563eb", "pembanding": "#dc2626", "ketiga": "#16a34a", "keempat": "#9333ea",
         "kelima": "#ea580c", "mentah": "#9ca3af", "target": "#111827"}

SINI = os.path.dirname(os.path.abspath(__file__))
KELUAR = os.path.join(SINI, "..", "gambar")


def jalankan():
    exe = os.path.join(tempfile.mkdtemp(), "sim")
    src = sorted(glob.glob(os.path.join(SINI, "..", "..", "src", "*.cpp")))
    subprocess.run(["g++", "-std=c++11", "-O2", "-I" + os.path.join(SINI, "..", "test"),
                    "-I" + os.path.join(SINI, "..", "..", "src"), os.path.join(SINI, "simulasi.cpp"), *src,
                    "-o", exe], check=True)
    keluaran = subprocess.run([exe], check=True, capture_output=True, text=True).stdout
    data, akhir, nama = {}, {}, None
    for baris in keluaran.splitlines():
        if baris.startswith("# akhir "):
            _, _, n, *kv = baris.split()
            akhir[n] = dict(x.split("=") for x in kv)
        elif baris.startswith("# "):
            nama = baris[2:]
            data[nama] = []
        elif baris[0].isdigit():
            data[nama].append([float(x) for x in baris.split(",")])
    return data, akhir


def kolom(baris, i):
    return [b[i] for b in baris]


def koma(x, d=1):
    return f"{x:.{d}f}".replace(".", ",")


def simpan(fig, nama):
    fig.savefig(os.path.join(KELUAR, nama), format="svg", metadata={"Date": None})
    plt.close(fig)


def main():
    os.makedirs(KELUAR, exist_ok=True)
    data, akhir = jalankan()
    for n, a in akhir.items():
        assert a["gagal"] == "0", f"{n}: perbarui() melaporkan data hilang"

    # 1. Belok 90 derajat ke kanan, diam, 180 derajat ke kiri, diam.
    b = data["belok"]
    t, benar, lib = kolom(b, 0), kolom(b, 1), kolom(b, 2)
    galat = max(abs(x - y) for x, y in zip(lib, benar))
    a = akhir["belok"]
    fig, ax = plt.subplots()
    ax.plot(t, lib, color=WARNA["utama"], label="sudutTotal() library")
    ax.plot(t, benar, "--", color=WARNA["target"], lw=1.4, label="arah sebenarnya")
    ax.set_title(f"Belok 90° kanan lalu 180° kiri: selisih terbesar {koma(galat)}°")
    ax.set_yticks([-90, 0, 90])
    ax.set_xlabel("waktu (detik)")
    ax.set_ylabel("sudut (°)")
    ax.text(3.6, 80, "belok kanan 90°", ha="left", va="top")
    ax.text(9.2, -80, "belok kiri 180°", ha="left", va="bottom")
    ax.annotate(f"arah() = {koma(float(a['arah']))}° ({a['mataAngin']})", (t[-1], lib[-1]), (t[-1], -40),
                ha="right", fontsize=9, color=WARNA["utama"], arrowprops=dict(arrowstyle="->", color=WARNA["utama"]))
    ax.legend(loc="lower left")
    simpan(fig, "arah_belok.svg")

    # 2. Koreksi bias otomatis saat bias gyro berubah karena suhu.
    on, off = data["bias_koreksi"], data["bias_tanpa_koreksi"]
    menit = [x / 60 for x in kolom(on, 0)]
    e_on = [x - y for x, y in zip(kolom(on, 2), kolom(on, 1))]
    e_off = [x - y for x, y in zip(kolom(off, 2), kolom(off, 1))]
    fig, ax = plt.subplots()
    benar = kolom(on, 1)
    for i in range(1, len(benar)):  # tandai saat berbelok
        if benar[i] != benar[i - 1]:
            ax.axvspan(menit[i - 1], menit[i], color=WARNA["mentah"], alpha=0.35, lw=0)
    ax.plot(menit, e_off, color=WARNA["pembanding"], label="aturKoreksiOtomatis(false)")
    ax.plot(menit, e_on, color=WARNA["utama"], label="aturKoreksiOtomatis(true), default")
    ax.axhline(0, color=WARNA["target"], ls="--", lw=1.0)
    ax.set_title(f"Bias gyro bergeser {koma(float(akhir['bias_koreksi']['biasZ']), 2)} °/s: setelah 10 menit "
                 f"drift {koma(abs(e_on[-1]))}°, tanpa koreksi {koma(abs(e_off[-1]))}°")
    ax.set_xlabel("waktu (menit)")
    ax.set_ylabel("selisih arah (°)")
    ax.text(menit[-1], e_off[-1] + 8, "tanpa koreksi", ha="right", va="bottom", color=WARNA["pembanding"])
    ax.text(menit[-1], e_on[-1] - 6, "koreksi otomatis (default)", ha="right", va="top", color=WARNA["utama"])
    ax.text(1.0, e_off[-1] * 0.85, "abu-abu = sedang berbelok", fontsize=8.5, color="#4b5563")
    simpan(fig, "arah_koreksi_bias.svg")

    # 3. Loop lambat: perbarui() tiap 300 ms vs baca gyro sesaat tiap 300 ms.
    b = data["loop_lambat"]
    t, benar, lib, naif = kolom(b, 0), kolom(b, 1), kolom(b, 2), kolom(b, 3)
    galat_lib = max(abs(x - y) for x, y in zip(lib, benar))
    fig, ax = plt.subplots()
    ax.plot(t, benar, "--", color=WARNA["target"], lw=1.4, label="arah sebenarnya", zorder=3)
    ax.plot(t, naif, color=WARNA["pembanding"], label="baca gyro sesaat tiap 300 ms")
    ax.plot(t, lib, color=WARNA["utama"], label="perbarui() tiap 300 ms (FIFO)")
    ax.set_title(f"Loop 300 ms: library tetap mengikuti (selisih maks. {koma(galat_lib)}°), "
                 f"gyro sesaat meleset {koma(abs(naif[-1] - benar[-1]))}°")
    ax.set_xlabel("waktu (detik)")
    ax.set_ylabel("sudut (°)")
    ax.set_yticks([-90, 0, 90, 180])
    ax.legend(loc="upper left", ncols=3, fontsize=9, bbox_to_anchor=(0, 1.0))
    ax.set_ylim(min(naif + benar) - 20, max(naif + benar) + 70)
    simpan(fig, "arah_loop_lambat.svg")


if __name__ == "__main__":
    sys.exit(main())
