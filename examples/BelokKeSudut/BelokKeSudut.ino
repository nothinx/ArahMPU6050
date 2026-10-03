// Robot berbelok ke sudut tertentu memakai selisihKe().
// Ketik sudut tujuan (0-360) di Serial Monitor lalu Enter.
//
// Ganti isi fungsi motorBelok() dan motorBerhenti() dengan driver motor
// yang dipakai (L298N, TB6612, servo kontinu, dll).
#include <ArahMPU6050.h>

const float TOLERANSI = 2.0;  // derajat, dianggap sudah sampai
const float KP = 3.0;         // penguatan: makin besar makin agresif
const int PWM_MIN = 60;       // PWM minimum agar motor mau bergerak
const int PWM_MAKS = 200;

ArahMPU6050 sensor;
float tujuan = 0;
bool berbelok = false;

void motorBelok(int pwm) {
  // pwm > 0 = putar kanan di tempat, pwm < 0 = putar kiri.
  // Contoh: roda kiri maju & roda kanan mundur untuk belok kanan.
  Serial.print("motor: ");
  Serial.println(pwm);
}

void motorBerhenti() {
  Serial.println("motor: berhenti");
}

void setup() {
  Serial.begin(115200);
  if (!sensor.mulai()) {
    Serial.println("MPU6050 tidak ditemukan!");
    while (true) delay(10);
  }
  while (!sensor.kalibrasi()) Serial.println("Sensor bergerak, ulangi kalibrasi...");
  Serial.println("Ketik sudut tujuan (0-360):");
}

void loop() {
  sensor.perbarui();

  if (Serial.available()) {
    tujuan = Serial.parseFloat();
    while (Serial.available()) Serial.read(); // buang sisa baris
    berbelok = true;
    Serial.print("Menuju ");
    Serial.println(tujuan);
  }

  if (!berbelok) return;

  static uint32_t terakhir = 0;
  if (millis() - terakhir < 50) return; // kendali 20 Hz
  terakhir = millis();

  float galat = sensor.selisihKe(tujuan);
  if (fabs(galat) <= TOLERANSI) {
    motorBerhenti();
    berbelok = false;
    Serial.print("Sampai di ");
    Serial.println(sensor.arah(), 1);
    return;
  }

  int pwm = constrain((int)(fabs(galat) * KP), PWM_MIN, PWM_MAKS);
  motorBelok(galat > 0 ? pwm : -pwm);
}
