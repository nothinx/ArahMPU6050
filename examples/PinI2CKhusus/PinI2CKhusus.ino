// Memakai pin I2C selain bawaan board.
// AVR (Uno/Nano/Mega) tidak bisa pindah pin I2C; pakai pin bawaan.
#include <ArahMPU6050.h>

ArahMPU6050 sensor;

void setup() {
  Serial.begin(115200);

#if defined(ARDUINO_ARCH_ESP32)
  Wire.setPins(16, 17);     // SDA, SCL (core ESP32 2.x ke atas)
  // Jalur I2C kedua: Wire1.setPins(25, 26); lalu sensor.mulai(Wire1);
#elif defined(ARDUINO_ARCH_STM32)
  Wire.setSDA(PB9);         // I2C1 remap di Blackpill/Bluepill
  Wire.setSCL(PB8);
#endif

  if (!sensor.mulai(Wire)) {
    Serial.println("MPU6050 tidak ditemukan, cek pin SDA/SCL!");
    while (true) delay(10);
  }

  // Kabel panjang (> 30 cm) atau tanpa resistor pull-up? Turunkan kecepatan I2C:
  // Wire.setClock(100000);

  while (!sensor.kalibrasi()) Serial.println("Sensor bergerak, ulangi kalibrasi...");
}

void loop() {
  sensor.perbarui();

  static uint32_t terakhir = 0;
  if (millis() - terakhir >= 200) {
    terakhir = millis();
    Serial.println(sensor.arah(), 1);
  }
}
