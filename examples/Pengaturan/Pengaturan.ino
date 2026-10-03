// Semua pengaturan yang tersedia, dengan nilai default-nya.
// Ubah sesuai kebutuhan project.
#include <ArahMPU6050.h>

ArahMPU6050 sensor;

void setup() {
  Serial.begin(115200);
  if (!sensor.mulai()) {
    Serial.println("MPU6050 tidak ditemukan!");
    while (true) delay(10);
  }

  // Rentang gyro: 250, 500, 1000, 2000 derajat/detik.
  // Robot yang berputar sangat cepat (mis. robot sumo) -> 1000 atau 2000.
  sensor.aturRentangGyro(500);

  // Putaran < ambang ini dianggap diam (untuk koreksi bias otomatis).
  sensor.aturAmbangDiam(1.0);

  // Koreksi bias otomatis saat diam. Matikan untuk benda yang berputar
  // sangat pelan & konstan (mis. meja putar), agar putarannya tidak hilang.
  sensor.aturKoreksiOtomatis(true);

  // Hasil dari contoh KalibrasiSkala.
  sensor.aturFaktorSkala(1.0);

  while (!sensor.kalibrasi()) Serial.println("Sensor bergerak, ulangi kalibrasi...");

  // Mulai dari arah tertentu, mis. robot diletakkan menghadap timur.
  sensor.aturArah(90);
}

void loop() {
  sensor.perbarui();

  static uint32_t terakhir = 0;
  if (millis() - terakhir >= 200) {
    terakhir = millis();
    Serial.print(sensor.arah(), 1);
    Serial.print("\t");
    Serial.print(sensor.mataAngin());
    Serial.println(sensor.diam() ? "\t(diam)" : "");
  }
}
