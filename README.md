# ArahMPU6050

Library Arduino berbahasa Indonesia untuk melacak **arah hadap (heading)** memakai IMU **MPU6050**: arah 0–360°, mata angin, selisih ke sudut tujuan, kecepatan putar, dan kemiringan.

Cocok untuk robot yang perlu berbelok dengan sudut tepat, line follower, robot KRSRI/KRI, odometri, penanda arah kendaraan, gimbal sederhana, dan sejenisnya.

> **Penting:** MPU6050 adalah IMU (akselerometer + gyro), **bukan kompas**. Sensor ini tidak memiliki magnetometer, jadi arahnya **relatif**: 0° = arah saat `mulai()` atau `resetArah()`. Untuk mengunci ke utara sungguhan, hadapkan sensor ke utara lalu panggil `resetArah()` (lihat contoh `ResetKeUtara`). Arah akan bergeser perlahan (drift) dalam hitungan menit; library ini menekannya, tapi tidak bisa menghilangkannya sepenuhnya.

## Fitur

- **Tahan loop lambat**: sensor menyimpan data 100 Hz di FIFO internal, jadi `delay()`, WiFi, atau LCD di `loop()` tidak membuat putaran hilang.
- **Kompensasi kemiringan**: sensor boleh miring atau dipasang tegak.
- **Koreksi bias otomatis** setiap sensor diam ≥ 1 detik, sehingga drift akibat perubahan suhu ikut terkoreksi.
- **Kalibrasi aman**: `kalibrasi()` menolak jika sensor tersenggol.
- **Koreksi skala** untuk gyro yang meleset 1–3%.
- Rentang gyro bisa diatur (250–2000 °/detik), dukungan **dua sensor** (0x68 & 0x69), **pin I2C khusus**, dan jalur I2C kedua (`Wire1`).
- Modul clone MPU6050 tetap terdeteksi.

## Board yang didukung

| Board | SDA | SCL | Teruji compile |
|---|---|---|---|
| Arduino Uno / Nano | A4 | A5 | ✅ |
| Arduino Mega | 20 | 21 | ✅ |
| ESP32 DevKit | 21 | 22 | ✅ |
| ESP32-C3 / S3 | sesuai board | sesuai board | ✅ |
| STM32 Blackpill F411 | PB7 | PB6 | ✅ |
| STM32 Bluepill F103 | PB7 | PB6 | ✅ |

STM32 memakai core resmi **STM32duino** (STMicroelectronics).

## Instalasi

**Library Manager:** Arduino IDE → *Sketch → Include Library → Manage Libraries…* → cari **ArahMPU6050** → *Install*.

**Manual:** unduh ZIP dari GitHub → *Sketch → Include Library → Add .ZIP Library…*

## Sambungan

| MPU6050 | Board |
|---|---|
| VCC | 3.3V atau 5V (modul GY-521 punya regulator) |
| GND | GND |
| SDA | SDA (lihat tabel di atas) |
| SCL | SCL |
| AD0 | GND/terbuka = 0x68, VCC = 0x69 |

## Contoh cepat

```cpp
#include <ArahMPU6050.h>

ArahMPU6050 sensor;

void setup() {
  Serial.begin(115200);
  if (!sensor.mulai()) {
    Serial.println("MPU6050 tidak ditemukan!");
    while (true) delay(10);
  }
  while (!sensor.kalibrasi()) Serial.println("Jangan gerakkan sensor...");
}

void loop() {
  sensor.perbarui();
  Serial.print(sensor.arah());
  Serial.print(" ");
  Serial.println(sensor.mataAngin());
  delay(100);
}
```

## Referensi fungsi

### Dasar

| Fungsi | Keterangan |
|---|---|
| `bool mulai(TwoWire &wire = Wire, uint8_t alamat = 0x68)` | Menyalakan sensor. `false` jika sensor tidak menjawab. |
| `bool kalibrasi(uint16_t sampel = 500)` | Mengukur bias gyro (±1 detik). Sensor harus diam; `false` jika bergerak. |
| `bool perbarui()` | Panggil di `loop()`, minimal tiap ±0,8 detik. `false` jika ada data hilang atau sensor tidak menjawab. |

### Arah

| Fungsi | Keterangan |
|---|---|
| `float arah()` | Arah hadap 0–360°, naik searah jarum jam. |
| `const char* mataAngin()` | `"Utara"`, `"Timur Laut"`, `"Timur"`, `"Tenggara"`, `"Selatan"`, `"Barat Daya"`, `"Barat"`, `"Barat Laut"`. |
| `const char* mataAnginSingkat()` | `"U"`, `"TL"`, `"T"`, `"TG"`, `"S"`, `"BD"`, `"B"`, `"BL"`. |
| `float selisihKe(float tujuan)` | Selisih terpendek ke sudut tujuan, −180…180. Positif = belok kanan. |
| `float sudutTotal()` | Total sudut tanpa dibungkus (720 = dua putaran ke kanan). |
| `void resetArah()` | Jadikan arah sekarang 0°. |
| `void aturArah(float derajat)` | Setel arah sekarang ke nilai tertentu. |

### Gerak & posisi

| Fungsi | Keterangan |
|---|---|
| `float kecepatanPutar()` | Kecepatan putar dalam °/detik. Positif = ke kanan. |
| `bool diam()` | `true` jika sensor diam ≥ 1 detik. |
| `float kemiringanDepan()` | Kemiringan sumbu X sensor, −90…90°. Positif = sisi X terangkat. |
| `float kemiringanSamping()` | Kemiringan sumbu Y sensor, −90…90°. Positif = sisi Y terangkat. |

### Pengaturan

| Fungsi | Default | Keterangan |
|---|---|---|
| `aturRentangGyro(uint16_t dps)` | 500 | 250 / 500 / 1000 / 2000. Naikkan untuk putaran sangat cepat. |
| `aturAmbangDiam(float dps)` | 1.0 | Putaran di bawah nilai ini dianggap diam. |
| `aturKoreksiOtomatis(bool aktif)` | `true` | Matikan untuk benda yang berputar sangat pelan & konstan (mis. meja putar). |
| `aturFaktorSkala(float faktor)` | 1.0 | Hasil dari contoh `KalibrasiSkala`. |
| `faktorSkala()` | | Membaca faktor skala saat ini. |

## Contoh yang tersedia

*File → Examples → ArahMPU6050*

| Contoh | Isi |
|---|---|
| `DasarArah` | Menampilkan arah dan mata angin. |
| `ResetKeUtara` | Tekan tombol saat menghadap utara agar mata angin sesuai kenyataan. |
| `BelokKeSudut` | Robot berbelok ke sudut tujuan dengan kendali proporsional. |
| `KalibrasiSkala` | Mengukur faktor skala dengan memutar sensor 10 kali. |
| `GrafikPlotter` | Arah, kecepatan putar, dan kemiringan di Serial Plotter. |
| `DuaSensor` | Dua MPU6050 di satu jalur I2C. |
| `PinI2CKhusus` | Pin I2C selain bawaan (ESP32, STM32) dan jalur `Wire1`. |
| `Pengaturan` | Semua pengaturan beserta nilai default-nya. |

## Tips akurasi

1. **Kalibrasi saat sensor benar-benar diam**, sebaiknya setelah sensor menyala ±10 detik agar suhunya stabil.
2. **Jalankan `KalibrasiSkala` sekali** per modul, lalu simpan faktornya di program.
3. **Pasang sensor kokoh.** Getaran motor yang merambat ke sensor menambah drift; beri peredam (busa/karet) jika perlu.
4. **Panggil `perbarui()` sesering mungkin**, minimal tiap ±0,8 detik.
5. **Kabel I2C panjang atau tidak stabil?** Panggil `Wire.setClock(100000);` setelah `mulai()`.

## Batasan

- Arah bersifat relatif dan akan drift seiring waktu. Untuk arah utara yang absolut dan stabil, tambahkan magnetometer (mis. QMC5883L/HMC5883L).
- Kemiringan dihitung dari akselerometer saja, sehingga kurang akurat saat sensor berakselerasi kuat.
- Satu objek `ArahMPU6050` memakai FIFO sensor; jangan memakai library MPU6050 lain pada sensor yang sama secara bersamaan.

## Lisensi

MIT © 2026 Amadeo Wisesa. Lihat [LICENSE](LICENSE).
