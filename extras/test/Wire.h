// Wire.h tiruan: bus I2C dengan satu MPU6050 tiruan di level register.
// Register yang ditiru: PWR_MGMT_1 (0x6B), SMPLRT_DIV (0x19), CONFIG (0x1A),
// GYRO_CONFIG (0x1B), ACCEL_CONFIG (0x1C), FIFO_EN (0x23), INT_ENABLE (0x38),
// INT_STATUS (0x3A), ACCEL_* (0x3B-0x40), GYRO_* (0x43-0x48), USER_CTRL (0x6A),
// FIFO_COUNT (0x72-0x73), FIFO_R_W (0x74), WHO_AM_I (0x75).
//
// Sensor membuat satu sampel setiap periode sample rate (DLPF aktif:
// 1 kHz / (1 + SMPLRT_DIV)) dari gerakan yang diatur program: putaran terhadap
// sumbu vertikal + bias gyro + noise Gauss, dan gravitasi pada akselerometer.
// Sampel masuk FIFO 1024 byte; saat penuh, byte tertua dibuang.
// ponytail: sampel = nilai sesaat, efek low-pass DLPF dan percepatan sentripetal
// tidak ditiru; cukup untuk logika FIFO/integrasi, bukan untuk meniru noise asli.
#pragma once
#include <Arduino.h>
#include <deque>
#include <math.h>
#include <string.h>

class TwoWire {
public:
  // --- Gerakan sensor, diatur program ---
  float (*laju)(float detik) = nullptr;  // putaran sebenarnya (derajat/detik), positif = ke kanan
  float (*biasZ)(float detik) = nullptr; // tambahan bias gyro Z yang berubah (mis. suhu), dps
  float bias[3] = {0.8f, -1.1f, 1.5f};   // bias tetap gyro X, Y, Z (dps)
  float noise = 0.05f;                   // simpangan baku noise gyro (dps)
  float noiseAksel = 0.005f;             // simpangan baku noise akselerometer (g)
  float atas[3] = {0, 0, 1};             // arah atas dalam sumbu sensor (vektor satuan) = kemiringan
  uint32_t benih = 1;                    // seed noise
  uint8_t alamat = 0x68;

  TwoWire() {
    memset(_reg, 0, sizeof(_reg));
    _reg[0x6B] = 0x40; // tidur setelah reset
    _reg[0x75] = 0x68; // WHO_AM_I
  }

  void begin() {}
  void setClock(uint32_t) {}
  void beginTransmission(uint8_t a) { _tujuan = a; _ntx = 0; }
  size_t write(uint8_t b) {
    if (_ntx >= sizeof(_tx)) return 0;
    _tx[_ntx++] = b;
    return 1;
  }
  uint8_t endTransmission(bool = true) {
    if (_tujuan != alamat) return 2; // alamat tidak dijawab (NACK)
    jalankan();
    if (_ntx) _ptr = _tx[0];
    for (uint8_t i = 1; i < _ntx; i++) tulisReg(_ptr++, _tx[i]);
    return 0;
  }
  uint8_t requestFrom(uint8_t a, uint8_t n) {
    _nrx = _irx = 0;
    if (a != alamat || n > sizeof(_rx)) return 0; // buffer Wire AVR 32 byte
    jalankan();
    for (uint8_t i = 0; i < n; i++) _rx[i] = bacaReg(_ptr == 0x74 ? _ptr : _ptr++);
    _nrx = n;
    return n;
  }
  int available() { return _nrx - _irx; }
  int read() { return _irx < _nrx ? _rx[_irx++] : -1; }

  // Isi FIFO saat ini (byte), untuk pemeriksaan program.
  size_t isiFifo() const { return _fifo.size(); }

private:
  uint8_t _reg[128], _tx[33], _rx[32];
  uint8_t _tujuan = 0, _ntx = 0, _nrx = 0, _irx = 0, _ptr = 0;
  uint32_t _tSampel = 0; // ms, sampel berikutnya
  std::deque<uint8_t> _fifo;

  void tulisReg(uint8_t r, uint8_t v) {
    if (r == 0x6A && (v & 0x04)) _fifo.clear(); // FIFO_RESET, bit kembali 0 sendiri
    if (r == 0x6A) v &= ~0x07;
    if (r < sizeof(_reg) && r != 0x75 && r != 0x3A) _reg[r] = v;
  }

  uint8_t bacaReg(uint8_t r) {
    if (r == 0x74) {
      if (_fifo.empty()) return 0;
      uint8_t b = _fifo.front();
      _fifo.pop_front();
      return b;
    }
    if (r == 0x72) return _fifo.size() >> 8;
    if (r == 0x73) return _fifo.size() & 0xFF;
    uint8_t v = r < sizeof(_reg) ? _reg[r] : 0;
    if (r == 0x3A) _reg[0x3A] = 0; // INT_STATUS bersih setelah dibaca
    return v;
  }

  // Buat semua sampel yang jatuh tempo sampai millis() sekarang.
  void jalankan() {
    uint32_t periode = 1 + _reg[0x19]; // ms, DLPF aktif (CONFIG 1..6)
    while ((int32_t)(millis() - _tSampel) >= 0) {
      if (!(_reg[0x6B] & 0x40)) sampel(_tSampel / 1000.0f);
      _tSampel += periode;
    }
  }

  float gauss() { // Box-Muller dari LCG, deterministik
    float u1 = (acak() + 1.0f) / 16777217.0f, u2 = acak() / 16777216.0f;
    return sqrtf(-2.0f * logf(u1)) * cosf(6.2831853f * u2);
  }
  uint32_t acak() { return (benih = benih * 1664525u + 1013904223u) >> 8; }

  static void simpan16(uint8_t *p, float v) {
    long x = lroundf(v);
    if (x > 32767) x = 32767;
    if (x < -32768) x = -32768;
    p[0] = (uint16_t)x >> 8;
    p[1] = (uint16_t)x & 0xFF;
  }

  void sampel(float t) {
    float r = laju ? laju(t) : 0;
    float lsbGyro = 131.0f / (1 << ((_reg[0x1B] >> 3) & 3));
    float lsbAksel = 16384.0f / (1 << ((_reg[0x1C] >> 3) & 3));
    for (uint8_t s = 0; s < 3; s++) {
      // Gyro positif = berlawanan jarum jam; putaran ke kanan terhadap arah atas.
      float w = -r * atas[s] + bias[s] + (s == 2 && biasZ ? biasZ(t) : 0) + noise * gauss();
      simpan16(_reg + 0x43 + 2 * s, w * lsbGyro);
      simpan16(_reg + 0x3B + 2 * s, (atas[s] + noiseAksel * gauss()) * lsbAksel);
    }
    if (!(_reg[0x6A] & 0x40)) return; // FIFO_EN di USER_CTRL
    uint8_t f = _reg[0x23];
    if (f & 0x08) tambahFifo(0x3B, 6); // urutan FIFO = urutan register
    if (f & 0x80) tambahFifo(0x41, 2);
    for (uint8_t s = 0; s < 3; s++)
      if (f & (0x40 >> s)) tambahFifo(0x43 + 2 * s, 2);
  }

  void tambahFifo(uint8_t r, uint8_t n) {
    for (uint8_t i = 0; i < n; i++) {
      if (_fifo.size() >= 1024) { // penuh: byte tertua dibuang
        _fifo.pop_front();
        // ponytail: bit FIFO_OFLOW_INT hanya diset bila INT_ENABLE mengizinkan
        // (perilaku chip asli tanpa INT_ENABLE tidak pasti, jadi pilih yang konservatif).
        if (_reg[0x38] & 0x10) _reg[0x3A] |= 0x10;
      }
      _fifo.push_back(_reg[r + i]);
    }
  }
};

extern TwoWire Wire;
