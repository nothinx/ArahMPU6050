// Hadapkan sensor ke utara sungguhan (pakai kompas HP), lalu tekan tombol.
// Setelah itu arah() dan mataAngin() mengikuti mata angin sebenarnya.
// Tombol dipasang antara PIN_TOMBOL dan GND.
#include <ArahMPU6050.h>

#if defined(ARDUINO_ARCH_STM32)
const int PIN_TOMBOL = PA0; // tombol KEY bawaan Blackpill
#elif defined(ARDUINO_ARCH_ESP32)
const int PIN_TOMBOL = 0;   // tombol BOOT bawaan ESP32 DevKit
#else
const int PIN_TOMBOL = 2;
#endif

ArahMPU6050 sensor;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_TOMBOL, INPUT_PULLUP);
  if (!sensor.mulai()) {
    Serial.println("MPU6050 tidak ditemukan!");
    while (true) delay(10);
  }
  while (!sensor.kalibrasi()) Serial.println("Sensor bergerak, ulangi kalibrasi...");
  Serial.println("Hadapkan ke utara lalu tekan tombol.");
}

void loop() {
  sensor.perbarui();

  if (digitalRead(PIN_TOMBOL) == LOW) {
    sensor.resetArah();
    Serial.println("Arah disetel: Utara = 0 derajat");
    delay(300); // debounce sederhana
  }

  static uint32_t terakhir = 0;
  if (millis() - terakhir >= 200) {
    terakhir = millis();
    Serial.print(sensor.mataAnginSingkat());
    Serial.print("\t");
    Serial.println(sensor.arah(), 1);
  }
}
