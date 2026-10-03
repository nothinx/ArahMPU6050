// ArahMPU6050 - Copyright (c) 2026 Amadeo Wisesa. Lisensi MIT.
#include "ArahMPU6050.h"
#include <math.h>

static const uint16_t LAJU_HZ = 100;
static const float DT = 1.0f / LAJU_HZ;
static const uint8_t BYTE_SAMPEL = 6;   // gyro X, Y, Z
static const uint8_t POTONGAN = 30;     // < buffer Wire AVR (32), kelipatan 6
static const float KE_DERAJAT = 57.2957795f;

static const char *const NAMA[8] = {"Utara", "Timur Laut", "Timur", "Tenggara",
                                    "Selatan", "Barat Daya", "Barat", "Barat Laut"};
static const char *const SINGKAT[8] = {"U", "TL", "T", "TG", "S", "BD", "B", "BL"};

static uint8_t indeksMataAngin(float a) { return (uint8_t)((a + 22.5f) / 45.0f) % 8; }
static int16_t be16(const uint8_t *p) { return (int16_t)((p[0] << 8) | p[1]); }

static float bungkus360(float d) {
  d = fmodf(d, 360.0f);
  return d < 0 ? d + 360.0f : d;
}

bool ArahMPU6050::mulai(TwoWire &wire, uint8_t alamat) {
  _wire = &wire;
  _alamat = alamat;
  _wire->begin();
  _wire->setClock(400000);
  _wire->beginTransmission(_alamat);
  if (_wire->endTransmission() != 0) return false; // sensor tidak menjawab

  tulis(0x6B, 0x01);               // bangunkan, clock dari gyro X
  delay(50);
  tulis(0x19, 1000 / LAJU_HZ - 1); // sample rate = 1 kHz / (1 + div)
  tulis(0x1A, 0x03);               // filter low-pass ~44 Hz
  tulis(0x1B, _fsSel << 3);        // rentang gyro
  tulis(0x1C, 0x00);               // akselerometer +-2 g
  tulis(0x23, 0x70);               // FIFO: gyro X, Y, Z
  delay(100);
  bacaAkselerasi(1.0f);
  resetFifo();
  return true;
}

bool ArahMPU6050::kalibrasi(uint16_t sampel) {
  if (!_wire || sampel == 0) return false;
  int32_t total[3] = {0, 0, 0};
  int16_t kecil[3] = {INT16_MAX, INT16_MAX, INT16_MAX};
  int16_t besar[3] = {INT16_MIN, INT16_MIN, INT16_MIN};
  for (uint16_t i = 0; i < sampel; i++) {
    uint8_t b[6];
    if (!baca(0x43, b, 6)) return false;
    for (uint8_t s = 0; s < 3; s++) {
      int16_t v = be16(b + 2 * s);
      total[s] += v;
      if (v < kecil[s]) kecil[s] = v;
      if (v > besar[s]) besar[s] = v;
    }
    delay(2);
  }
  for (uint8_t s = 0; s < 3; s++)
    if (besar[s] - kecil[s] > 3 * _lsb) return false; // bergerak > 3 dps

  for (uint8_t s = 0; s < 3; s++) _bias[s] = (float)total[s] / sampel;
  bacaAkselerasi(1.0f);
  _hitungDiam = 0;
  resetFifo();
  return true;
}

bool ArahMPU6050::perbarui() {
  if (!_wire) return false;
  uint8_t status, c[2];
  if (!baca(0x3A, &status, 1) || !baca(0x72, c, 2)) return false;
  uint16_t n = (c[0] << 8) | c[1];

  // Overflow / data tidak sejajar: buang FIFO, putaran selama itu hilang.
  if ((status & 0x10) || n % BYTE_SAMPEL) {
    resetFifo();
    return false;
  }
  if (n == 0) return true;

  bacaAkselerasi(0.2f);
  uint8_t buf[POTONGAN];
  while (n) {
    uint8_t k = n > POTONGAN ? POTONGAN : n;
    if (!baca(0x74, buf, k)) return false;
    for (uint8_t i = 0; i < k; i += BYTE_SAMPEL) proses(buf + i);
    n -= k;
  }
  return true;
}

void ArahMPU6050::proses(const uint8_t *p) {
  float mentah[3], w[3];
  bool tenang = true;
  for (uint8_t s = 0; s < 3; s++) {
    mentah[s] = be16(p + 2 * s);
    w[s] = (mentah[s] - _bias[s]) / _lsb * _skala;
    if (fabsf(w[s]) >= _ambang) tenang = false;
  }

  if (!tenang) {
    _hitungDiam = 0;
  } else if (_hitungDiam < LAJU_HZ) {
    _hitungDiam++;
  } else if (_koreksiOtomatis) {
    // Diam >= 1 detik: koreksi bias perlahan, jangan integrasikan noise.
    for (uint8_t s = 0; s < 3; s++) _bias[s] += 0.01f * (mentah[s] - _bias[s]);
    _laju = 0;
    return;
  }

  // Putaran terhadap sumbu vertikal = kompensasi kemiringan.
  // Gyro positif = berlawanan jarum jam; dibalik agar positif = ke kanan.
  _laju = -(w[0] * _atas[0] + w[1] * _atas[1] + w[2] * _atas[2]);
  float d = _laju * DT;
  _total += d;
  _arah = bungkus360(_arah + d);
}

void ArahMPU6050::aturArah(float derajat) {
  _total = derajat;
  _arah = bungkus360(derajat);
}

float ArahMPU6050::selisihKe(float tujuan) const {
  float e = bungkus360(tujuan - _arah);
  return e > 180.0f ? e - 360.0f : e;
}

bool ArahMPU6050::diam() const { return _hitungDiam >= LAJU_HZ; }

const char *ArahMPU6050::mataAngin() const { return NAMA[indeksMataAngin(_arah)]; }
const char *ArahMPU6050::mataAnginSingkat() const { return SINGKAT[indeksMataAngin(_arah)]; }

float ArahMPU6050::kemiringanDepan() const {
  return atan2f(_atas[0], sqrtf(_atas[1] * _atas[1] + _atas[2] * _atas[2])) * KE_DERAJAT;
}

float ArahMPU6050::kemiringanSamping() const {
  return atan2f(_atas[1], sqrtf(_atas[0] * _atas[0] + _atas[2] * _atas[2])) * KE_DERAJAT;
}

void ArahMPU6050::aturRentangGyro(uint16_t dps) {
  uint8_t fs = dps <= 250 ? 0 : dps <= 500 ? 1 : dps <= 1000 ? 2 : 3;
  float lsb = 131.0f / (1 << fs);
  for (uint8_t s = 0; s < 3; s++) _bias[s] *= lsb / _lsb; // bias ikut skala baru
  _fsSel = fs;
  _lsb = lsb;
  if (_wire) {
    tulis(0x1B, _fsSel << 3);
    resetFifo(); // buang sampel dengan rentang lama
  }
}

void ArahMPU6050::bacaAkselerasi(float alfa) {
  uint8_t b[6];
  if (!baca(0x3B, b, 6)) return;
  float a[3] = {(float)be16(b), (float)be16(b + 2), (float)be16(b + 4)};
  float g = sqrtf(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
  if (g < 0.7f * 16384 || g > 1.3f * 16384) return; // sedang berakselerasi, abaikan

  float m = 0;
  for (uint8_t s = 0; s < 3; s++) {
    _atas[s] += alfa * (a[s] / g - _atas[s]);
    m += _atas[s] * _atas[s];
  }
  m = sqrtf(m);
  for (uint8_t s = 0; s < 3; s++) _atas[s] /= m;
}

void ArahMPU6050::resetFifo() {
  tulis(0x6A, 0x04); // FIFO_RESET
  tulis(0x6A, 0x40); // FIFO_EN
}

bool ArahMPU6050::baca(uint8_t reg, uint8_t *buf, uint8_t n) {
  _wire->beginTransmission(_alamat);
  _wire->write(reg);
  if (_wire->endTransmission(false) != 0) return false;
  if (_wire->requestFrom(_alamat, n) != n) return false;
  for (uint8_t i = 0; i < n; i++) buf[i] = _wire->read();
  return true;
}

void ArahMPU6050::tulis(uint8_t reg, uint8_t nilai) {
  _wire->beginTransmission(_alamat);
  _wire->write(reg);
  _wire->write(nilai);
  _wire->endTransmission();
}
