// Dua MPU6050 di satu jalur I2C.
// Sensor A: pin AD0 ke GND (alamat 0x68)
// Sensor B: pin AD0 ke VCC (alamat 0x69)
#include <ArahMPU6050.h>

ArahMPU6050 sensorA;
ArahMPU6050 sensorB;

void setup() {
  Serial.begin(115200);
  bool okA = sensorA.mulai(Wire, 0x68);
  bool okB = sensorB.mulai(Wire, 0x69);
  if (!okA || !okB) {
    Serial.print("Sensor tidak ditemukan:");
    if (!okA) Serial.print(" A(0x68)");
    if (!okB) Serial.print(" B(0x69)");
    Serial.println();
    while (true) delay(10);
  }
  while (!sensorA.kalibrasi()) Serial.println("A bergerak, ulangi...");
  while (!sensorB.kalibrasi()) Serial.println("B bergerak, ulangi...");
}

void loop() {
  sensorA.perbarui();
  sensorB.perbarui();

  static uint32_t terakhir = 0;
  if (millis() - terakhir >= 200) {
    terakhir = millis();
    Serial.print("A: ");
    Serial.print(sensorA.arah(), 1);
    Serial.print("\tB: ");
    Serial.print(sensorB.arah(), 1);
    Serial.print("\tSelisih: ");
    Serial.println(sensorA.selisihKe(sensorB.arah()), 1);
  }
}
