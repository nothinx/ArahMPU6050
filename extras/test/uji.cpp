// Uji logika ArahMPU6050 di PC dengan MPU6050 tiruan di level register (Wire.h):
//   g++ -std=c++11 -Wall -Wextra -I. -I../../src uji.cpp ../../src/ArahMPU6050.cpp -o uji && ./uji
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "ArahMPU6050.h"

uint32_t waktuPalsu = 0;
TwoWire Wire;

static bool dekat(float a, float b, float tol) { return fabsf(a - b) <= tol; }

// Putaran tetap selama [mulaiPutar, mulaiPutar + lamaPutar) detik.
static float lajuTetap = 0, mulaiPutar = 1e9f, lamaPutar = 0;
static float laju(float t) { return t >= mulaiPutar && t < mulaiPutar + lamaPutar ? lajuTetap : 0; }

// Sensor baru, sudah mulai() dan kalibrasi() dalam keadaan diam.
static void siapkan(ArahMPU6050 &s) {
  Wire = TwoWire();
  Wire.laju = laju;
  waktuPalsu = 0;
  mulaiPutar = 1e9f;
  assert(s.mulai() && s.kalibrasi());
}

// Putar lajuDps selama detik, perbarui() setiap selangMs. Mengembalikan false
// jika ada perbarui() yang gagal.
static bool putar(ArahMPU6050 &s, float lajuDps, float detik, uint32_t selangMs) {
  lajuTetap = lajuDps;
  mulaiPutar = waktuPalsu / 1000.0f;
  lamaPutar = detik;
  bool semua = true;
  uint32_t akhir = waktuPalsu + (uint32_t)(detik * 1000) + 200; // + sisa sampel terakhir
  while (waktuPalsu < akhir) {
    waktuPalsu += selangMs;
    semua &= s.perbarui();
  }
  return semua;
}

int main() {
  int kasus = 0;
  { // alamat salah: mulai() gagal; tanpa mulai(): perbarui() false
    ArahMPU6050 s;
    assert(!s.perbarui());
    Wire = TwoWire();
    assert(!s.mulai(Wire, 0x69));
    kasus++;
  }
  { // kalibrasi menolak sensor yang digoyang (rentang bacaan > 3 dps)
    ArahMPU6050 s;
    Wire = TwoWire();
    Wire.laju = [](float t) { return 30 * sinf(10 * t); };
    waktuPalsu = 0;
    assert(s.mulai() && !s.kalibrasi());
    kasus++;
  }
  { // belok 90 derajat ke kanan dalam 1 detik, lalu 180 ke kiri: arah, sudutTotal, mata angin
    ArahMPU6050 s;
    siapkan(s);
    assert(dekat(s.arah(), 0, 0.01f) && !strcmp(s.mataAngin(), "Utara"));
    assert(putar(s, 90, 1, 10));
    assert(dekat(s.arah(), 90, 1) && !strcmp(s.mataAngin(), "Timur") && !strcmp(s.mataAnginSingkat(), "T"));
    assert(putar(s, -90, 2, 10));
    assert(dekat(s.arah(), 270, 1.5f) && dekat(s.sudutTotal(), -90, 1.5f) && !strcmp(s.mataAngin(), "Barat"));
    kasus++;
  }
  { // loop lambat: perbarui() tiap 1,5 detik tidak kehilangan putaran (FIFO 1024 B = 1,7 s)
    ArahMPU6050 s;
    siapkan(s);
    assert(putar(s, 60, 6, 1500));
    assert(dekat(s.arah(), 360 - 0.5f, 3) || dekat(s.arah(), 0, 3)); // 6 s x 60 dps = 360
    assert(dekat(s.sudutTotal(), 360, 3));
    kasus++;
  }
  { // loop terlalu lambat (2 detik): FIFO meluap, perbarui() melapor false
    ArahMPU6050 s;
    siapkan(s);
    waktuPalsu += 2000;
    assert(!s.perbarui());
    waktuPalsu += 100;
    assert(s.perbarui()); // FIFO sudah di-reset, berjalan lagi
    kasus++;
  }
  { // sensor dipasang miring 30 derajat: putaran terhadap sumbu vertikal tetap terbaca penuh
    ArahMPU6050 s;
    Wire = TwoWire();
    Wire.laju = laju;
    Wire.atas[0] = sinf(30 / 57.2957795f);
    Wire.atas[2] = cosf(30 / 57.2957795f);
    waktuPalsu = 0;
    mulaiPutar = 1e9f;
    assert(s.mulai() && s.kalibrasi());
    assert(dekat(s.kemiringanDepan(), 30, 1) && dekat(s.kemiringanSamping(), 0, 1));
    assert(putar(s, 90, 1, 10));
    assert(dekat(s.arah(), 90, 1.5f));
    kasus++;
  }
  { // diam lama dengan bias yang berubah: koreksi otomatis menahan drift
    ArahMPU6050 s;
    siapkan(s);
    Wire.bias[2] += 0.3f; // bias bergeser setelah kalibrasi
    assert(putar(s, 0, 60, 20));
    assert(s.diam() && fabsf(s.selisihKe(0)) < 1.5f && s.kecepatanPutar() == 0);
    kasus++;
  }
  { // aturArah, selisihKe, rentang gyro
    ArahMPU6050 s;
    siapkan(s);
    s.aturArah(-30);
    assert(dekat(s.arah(), 330, 0.01f) && dekat(s.sudutTotal(), -30, 0.01f));
    assert(dekat(s.selisihKe(10), 40, 0.01f) && dekat(s.selisihKe(300), -30, 0.01f));
    s.aturRentangGyro(2000); // 600 dps melewati batas rentang default 500
    assert(putar(s, 600, 0.5f, 10));
    assert(dekat(s.sudutTotal(), -30 + 300, 3));
    kasus++;
  }
  printf("Semua uji lolos (%d kasus, sizeof = %u byte)\n", kasus, (unsigned)sizeof(ArahMPU6050));
  return 0;
}
