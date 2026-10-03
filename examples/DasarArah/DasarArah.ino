// Contoh dasar: tampilkan arah hadap dan mata angin di Serial Monitor (115200).
//
// Sambungan MPU6050: VCC -> 3.3V/5V, GND -> GND, SDA/SCL -> pin I2C board
//   Uno/Nano      : SDA = A4,  SCL = A5
//   Mega          : SDA = 20,  SCL = 21
//   ESP32         : SDA = 21,  SCL = 22
//   Blackpill F411: SDA = PB7, SCL = PB6
//   Bluepill F103 : SDA = PB7, SCL = PB6
#include <ArahMPU6050.h>

ArahMPU6050 sensor;

void setup() {
  Serial.begin(115200);
  if (!sensor.mulai()) {
    Serial.println("MPU6050 tidak ditemukan, cek kabel!");
    while (true) delay(10);
  }
  Serial.println("Kalibrasi, jangan gerakkan sensor...");
  while (!sensor.kalibrasi()) Serial.println("Sensor bergerak, ulangi kalibrasi...");
  Serial.println("Siap! Arah saat ini = 0 derajat.");
}

void loop() {
  if (!sensor.perbarui()) Serial.println("Peringatan: data hilang (loop terlalu lambat?)");

  static uint32_t terakhir = 0;
  if (millis() - terakhir >= 200) {
    terakhir = millis();
    Serial.print("Arah: ");
    Serial.print(sensor.arah(), 1);
    Serial.print(" derajat  ");
    Serial.println(sensor.mataAngin());
  }
}
