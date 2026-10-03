// Arduino.h tiruan untuk menjalankan ArahMPU6050 di PC (lihat Wire.h).
// Waktu dikendalikan program: waktuPalsu (ms) didefinisikan oleh program.
#pragma once
#include <stddef.h>
#include <stdint.h>
extern uint32_t waktuPalsu;
inline uint32_t millis() { return waktuPalsu; }
inline void delay(uint32_t ms) { waktuPalsu += ms; }
