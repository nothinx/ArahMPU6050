// Simulasi ArahMPU6050 di PC: kode library asli membaca MPU6050 tiruan
// (../test/Wire.h) yang mengisi FIFO 100 Hz dari gerakan sintetis.
// Keluaran CSV per skenario ke stdout, dibaca oleh gambar.py.
#include <stdio.h>
#include <stdlib.h>
#include "ArahMPU6050.h"

uint32_t waktuPalsu = 0;
TwoWire Wire;

// Gerakan: daftar belokan dengan profil kecepatan halus (1 - cos),
// waktu dalam detik sejak t0 (selesai kalibrasi).
struct Belok { float mulai, lama, sudut; };
static const Belok *belok;
static int nBelok;
static float t0;

static float lajuSebenarnya(float t) {
  float r = 0;
  for (int i = 0; i < nBelok; i++) {
    float u = (t - t0 - belok[i].mulai) / belok[i].lama;
    if (u > 0 && u < 1) r += belok[i].sudut / belok[i].lama * (1 - cosf(6.2831853f * u));
  }
  return r;
}

static double sudutSebenarnya(double t) { // integral eksak lajuSebenarnya
  double a = 0;
  for (int i = 0; i < nBelok; i++) {
    double u = (t - belok[i].mulai) / belok[i].lama;
    if (u >= 1) a += belok[i].sudut;
    else if (u > 0) a += belok[i].sudut * (u - sin(6.283185307 * u) / 6.283185307);
  }
  return a;
}

// Bias gyro Z naik perlahan 0,4 dps (sensor menghangat, konstanta waktu 3 menit).
static float biasHangat(float t) { return t > t0 ? 0.4f * (1 - expf(-(t - t0) / 180.0f)) : 0; }

// Jalankan satu skenario. loopMs = selang antar perbarui() (loop dengan delay).
// pembanding: juga hitung arah dari register gyro sesaat tiap loop
// (arah += laju_sesaat * selang), cara yang umum dipakai di tutorial.
static void jalan(const char *nama, const Belok *b, int n, float detik, uint32_t loopMs, uint32_t cetakMs,
                  bool koreksi, float (*biasZ)(float), bool pembanding) {
  belok = b;
  nBelok = n;
  Wire = TwoWire();
  Wire.laju = lajuSebenarnya;
  Wire.biasZ = biasZ;
  waktuPalsu = 0;
  t0 = 1e9f; // belum ada gerakan selama mulai() dan kalibrasi

  ArahMPU6050 sensor;
  if (!sensor.mulai() || !sensor.kalibrasi()) {
    fprintf(stderr, "%s: mulai()/kalibrasi() gagal\n", nama);
    exit(1);
  }
  sensor.aturKoreksiOtomatis(koreksi);

  // Pembanding dikalibrasi dengan cara yang sama: rata-rata 500 bacaan gyro Z.
  float biasNaif = 0, arahNaif = 0;
  if (pembanding) {
    long total = 0;
    for (int i = 0; i < 500; i++) {
      uint8_t p[2];
      Wire.beginTransmission(0x68); Wire.write(0x47); Wire.endTransmission(false);
      Wire.requestFrom(0x68, 2);
      p[0] = Wire.read(); p[1] = Wire.read();
      total += (int16_t)(p[0] << 8 | p[1]);
      delay(2);
    }
    biasNaif = total / 500.0f;
    sensor.perbarui(); // buang sampel FIFO selama kalibrasi pembanding
    sensor.aturArah(0);
  }

  uint32_t awal = millis();
  t0 = awal / 1000.0f;
  int gagal = 0;
  printf("# %s\n", nama);
  printf(pembanding ? "t,sebenarnya,library,pembanding\n" : "t,sebenarnya,library\n");
  for (uint32_t t = 0; t <= (uint32_t)(detik * 1000); t += loopMs) {
    waktuPalsu = awal + t;
    if (!sensor.perbarui()) gagal++;
    if (pembanding && t > 0) {
      uint8_t p[2];
      Wire.beginTransmission(0x68); Wire.write(0x47); Wire.endTransmission(false);
      Wire.requestFrom(0x68, 2);
      p[0] = Wire.read(); p[1] = Wire.read();
      arahNaif += -((int16_t)(p[0] << 8 | p[1]) - biasNaif) / 65.5f * (loopMs / 1000.0f);
    }
    if (t % cetakMs == 0) {
      printf("%.2f,%.3f,%.3f", t / 1000.0, sudutSebenarnya(t / 1000.0), sensor.sudutTotal());
      if (pembanding) printf(",%.3f", arahNaif);
      printf("\n");
    }
  }
  printf("# akhir %s arah=%.1f mataAngin=%s gagal=%d biasZ=%.2f\n", nama, sensor.arah(), sensor.mataAngin(), gagal,
         biasZ ? biasZ(t0 + detik) : 0.0f);
}

int main() {
  // 1. Belok 90 derajat ke kanan, diam, belok 180 derajat ke kiri, diam.
  static const Belok belokDasar[] = {{2, 1.5f, 90}, {6.5f, 2.5f, -180}};
  jalan("belok", belokDasar, 2, 12, 10, 50, true, nullptr, false);

  // 2. Bias gyro berubah karena suhu selama 10 menit, hampir selalu diam.
  static const Belok belokJarang[] = {{60, 2, 90}, {200, 3, -180}, {330, 2, 90}, {470, 2, 90}};
  jalan("bias_koreksi", belokJarang, 4, 600, 10, 1000, true, biasHangat, false);
  jalan("bias_tanpa_koreksi", belokJarang, 4, 600, 10, 1000, false, biasHangat, false);

  // 3. Loop lambat: perbarui() tiap 300 ms, belokan cepat 0,4-0,8 detik (puncak <= 450 dps).
  static const Belok belokCepat[] = {{1.13f, 0.4f, 90},  {3.41f, 0.5f, -90}, {5.07f, 0.4f, 90},
                                     {7.52f, 0.5f, 90},  {9.86f, 0.8f, -180}, {12.3f, 0.4f, 90}};
  jalan("loop_lambat", belokCepat, 6, 15, 300, 300, true, nullptr, true);
  return 0;
}
