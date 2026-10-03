// Benchmark ArahMPU6050 di board sungguhan dengan MPU6050 terpasang (SDA/SCL).
// Angka di README (bagian "Kecepatan & memori") diukur di simulator simavr
// tanpa sensor; sketch ini mengukur hal yang sama ditambah waktu I2C.
// Buka Serial Monitor 115200, biarkan sensor diam lalu putar perlahan.
#include <ArahMPU6050.h>

ArahMPU6050 imu;

void setup() {
  Serial.begin(115200);
  if (!imu.mulai() || !imu.kalibrasi()) {
    Serial.println(F("MPU6050 tidak terbaca atau bergerak saat kalibrasi"));
    while (true) {}
  }
  Serial.print(F("RAM objek: "));
  Serial.print(sizeof(ArahMPU6050));
  Serial.println(F(" byte"));
}

void loop() {
  // Tunggu 100 ms (10 sampel di FIFO), lalu ukur satu perbarui().
  delay(100);
  uint32_t t = micros();
  imu.perbarui();
  t = micros() - t;
  Serial.print(F("perbarui() 10 sampel: "));
  Serial.print(t);
  Serial.print(F(" us, arah "));
  Serial.println(imu.arah(), 1);
}
