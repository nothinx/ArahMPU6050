// Mengukur faktor skala gyro. Gyro & clock sensor biasanya meleset 1-3%,
// sehingga putaran 360 derajat terbaca misalnya 352 derajat.
//
// Cara:
//  1. Letakkan sensor di permukaan datar, buka Serial Monitor (115200).
//  2. Ketik r lalu Enter -> sudut direset.
//  3. Putar sensor tepat 10 kali searah jarum jam, kembali ke posisi awal.
//  4. Ketik s lalu Enter -> faktor skala ditampilkan.
//  5. Pakai di program: sensor.aturFaktorSkala(<faktor>);
#include <ArahMPU6050.h>

ArahMPU6050 sensor;

void setup() {
  Serial.begin(115200);
  if (!sensor.mulai()) {
    Serial.println("MPU6050 tidak ditemukan!");
    while (true) delay(10);
  }
  while (!sensor.kalibrasi()) Serial.println("Sensor bergerak, ulangi kalibrasi...");
  Serial.println("Ketik r untuk mulai.");
}

void loop() {
  sensor.perbarui();

  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'r') {
      sensor.resetArah();
      Serial.println("Putar tepat 10 kali searah jarum jam, lalu ketik s.");
    } else if (c == 's') {
      float total = sensor.sudutTotal();
      if (total < 1800) {
        Serial.println("Putaran terlalu sedikit atau terbalik arah. Ulangi dengan r.");
        return;
      }
      float faktor = sensor.faktorSkala() * 3600.0 / total;
      Serial.print("Terbaca ");
      Serial.print(total, 1);
      Serial.println(" dari 3600 derajat.");
      Serial.print("sensor.aturFaktorSkala(");
      Serial.print(faktor, 4);
      Serial.println(");");
    }
  }

  static uint32_t terakhir = 0;
  if (millis() - terakhir >= 500) {
    terakhir = millis();
    Serial.print("Total: ");
    Serial.println(sensor.sudutTotal(), 1);
  }
}
