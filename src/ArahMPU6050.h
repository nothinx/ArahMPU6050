// ArahMPU6050 - pelacak arah hadap (heading) relatif dari IMU MPU6050.
// Copyright (c) 2026 Amadeo Wisesa. Lisensi MIT.
//
// MPU6050 adalah IMU (akselerometer + gyro), bukan kompas: tidak ada
// magnetometer, jadi 0 derajat = arah saat mulai() atau resetArah().
//
// - Sampling tetap 100 Hz lewat FIFO sensor: loop() yang lambat tidak membuat
//   putaran hilang, asal perbarui() dipanggil minimal tiap ~0.8 detik.
// - Kompensasi kemiringan: sensor boleh miring atau dipasang tegak.
// - Bias gyro dikoreksi otomatis setiap sensor diam >= 1 detik.
#pragma once
#include <Arduino.h>
#include <Wire.h>

class ArahMPU6050 {
public:
  // Pin I2C khusus: panggil Wire.setPins(sda, scl) (ESP32) atau
  // Wire.setSDA()/Wire.setSCL() (STM32) sebelum mulai().
  // alamat: 0x68 (pin AD0 ke GND/terbuka) atau 0x69 (AD0 ke VCC).
  bool mulai(TwoWire &wire = Wire, uint8_t alamat = 0x68);

  // Sensor harus diam. Mengembalikan false jika sensor bergerak (ulangi).
  bool kalibrasi(uint16_t sampel = 500);

  // Panggil di loop(). Mengembalikan false jika data hilang
  // (loop terlalu lambat) atau sensor tidak menjawab.
  bool perbarui();

  // --- Arah ---
  float arah() const { return _arah; }   // 0..360, naik searah jarum jam
  const char *mataAngin() const;         // "Utara", "Timur Laut", ...
  const char *mataAnginSingkat() const;  // "U", "TL", ...
  // Selisih terpendek ke arah tujuan, -180..180.
  // Positif = belok kanan (searah jarum jam), negatif = belok kiri.
  float selisihKe(float tujuan) const;
  float sudutTotal() const { return _total; } // tanpa dibungkus 0..360
  void resetArah() { aturArah(0); }
  void aturArah(float derajat);

  // --- Gerak & posisi ---
  float kecepatanPutar() const { return _laju; } // derajat/detik, positif = ke kanan
  bool diam() const;
  // Kemiringan dari akselerometer, -90..90 derajat.
  // Positif = sisi sumbu X (depan) / sumbu Y (samping) sensor terangkat.
  float kemiringanDepan() const;
  float kemiringanSamping() const;

  // --- Pengaturan ---
  // 250, 500 (default), 1000, atau 2000 derajat/detik.
  // Rentang besar = putaran cepat tidak mentok, tapi resolusi lebih kasar.
  void aturRentangGyro(uint16_t dps);
  // Putaran di bawah ambang ini (per sumbu) dianggap diam. Default 1 dps.
  void aturAmbangDiam(float dps) { _ambang = dps; }
  // Matikan untuk benda yang berputar sangat pelan & konstan (< ambang diam),
  // misalnya meja putar, karena putarannya akan dianggap bias.
  void aturKoreksiOtomatis(bool aktif) { _koreksiOtomatis = aktif; }
  // Koreksi skala, lihat contoh KalibrasiSkala. Default 1.0.
  void aturFaktorSkala(float faktor) { _skala = faktor; }
  float faktorSkala() const { return _skala; }

private:
  void proses(const uint8_t *p);
  void bacaAkselerasi(float alfa);
  void resetFifo();
  bool baca(uint8_t reg, uint8_t *buf, uint8_t n);
  void tulis(uint8_t reg, uint8_t nilai);

  TwoWire *_wire = nullptr;
  uint8_t _alamat = 0x68;
  uint8_t _fsSel = 1;          // 0..3 -> +-250..2000 dps
  float _lsb = 65.5f;          // LSB per derajat/detik
  float _bias[3] = {0, 0, 0};
  float _atas[3] = {0, 0, 1};  // arah ke atas (vektor satuan dari gravitasi)
  float _arah = 0, _total = 0, _laju = 0, _ambang = 1.0f, _skala = 1.0f;
  uint16_t _hitungDiam = 0;
  bool _koreksiOtomatis = true;
};
