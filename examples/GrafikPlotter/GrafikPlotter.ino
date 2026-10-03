// Tampilkan arah, kecepatan putar, dan kemiringan di Serial Plotter
// (Tools > Serial Plotter, 115200).
#include <ArahMPU6050.h>

ArahMPU6050 sensor;

void setup() {
  Serial.begin(115200);
  if (!sensor.mulai()) {
    Serial.println("MPU6050 tidak ditemukan!");
    while (true) delay(10);
  }
  while (!sensor.kalibrasi()) delay(100);
}

void loop() {
  sensor.perbarui();

  static uint32_t terakhir = 0;
  if (millis() - terakhir >= 50) {
    terakhir = millis();
    Serial.print("arah:");
    Serial.print(sensor.arah(), 1);
    Serial.print("\tputar:");
    Serial.print(sensor.kecepatanPutar(), 1);
    Serial.print("\tmiringDepan:");
    Serial.print(sensor.kemiringanDepan(), 1);
    Serial.print("\tmiringSamping:");
    Serial.println(sensor.kemiringanSamping(), 1);
  }
}
