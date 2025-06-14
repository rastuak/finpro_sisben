# Sistem Pemantauan Dispenser Otomatis

Proyek ini menyediakan sistem komprehensif untuk memantau level dan penggunaan air dari dispenser otomatis. Sistem ini mengintegrasikan broker MQTT untuk penerimaan data secara *real-time*, basis data MongoDB untuk penyimpanan data, dan *bot* Discord untuk menampilkan pembaruan status serta grafik historis.

---

## Fitur Utama

* **Pemantauan Level Air *Real-time***: Dapatkan pembaruan instan mengenai level air dispenser Anda langsung di Discord.
* **Pelacakan Penggunaan Air**: Pantau berapa banyak air yang dikeluarkan, termasuk volume dan durasi setiap penggunaan.
* **Visualisasi Data Historis**: Hasilkan grafik level dan penggunaan air dari waktu ke waktu.
* **Integrasi Discord**: Semua interaksi dan notifikasi dilakukan melalui *bot* Discord dan *webhook*.
* **Penyimpanan MongoDB**: Simpan semua data sensor dengan aman untuk analisis lebih lanjut.

---

## Struktur Proyek

Proyek ini terdiri dari dua skrip Python utama:

* `main.py`: Skrip ini menangani komunikasi MQTT, menerima data dari dispenser, menyimpannya di MongoDB, dan mengirimkan pembaruan *real-time* ke Discord melalui *webhook*.
* `bot.py`: Skrip ini menjalankan *bot* Discord, memungkinkan pengguna untuk menanyakan status level air saat ini dan menghasilkan grafik historis dari data yang tersimpan di MongoDB.

---

## Penyiapan dan Instalasi

### Prasyarat

Sebelum memulai, pastikan Anda memiliki hal-hal berikut:

1.  **Python 3.8+** terinstal.
2.  **Broker MQTT** (misalnya, Mosquitto, HiveMQ). Anda akan membutuhkan alamat, *port*, *username*, dan *password*-nya.
3.  **MongoDB Atlas Cluster** (direkomendasikan) atau instans MongoDB lokal. Anda akan membutuhkan URI koneksinya.
4.  **Server Discord** tempat Anda ingin menerapkan *bot*.
5.  **Token *Bot* Discord** dan **URL *Webhook***.

### Langkah-langkah Instalasi

#### 1. Klon Repositori

```bash
git clone https://github.com/rastuak/finpro_sisben
cd finpro_sisben
```

#### 2. Instal Dependensi

```bash
pip install -r requirements.txt
```

Jika Anda belum memiliki file `requirements.txt`, buatlah dengan konten berikut:

```
discord.py
pymongo
paho-mqtt
requests
matplotlib
```

Kemudian jalankan:

```bash
pip install -r requirements.txt
```

#### 3. Konfigurasi `main.py`

Buka `main.py` dan ganti nilai *placeholder* dengan kredensial Anda yang sebenarnya:

* **`MONGO_URI`**: *String* koneksi MongoDB Anda.
* **`MQTT_BROKER`**: Alamat broker MQTT Anda.
* **`MQTT_USERNAME`**: *Username* MQTT Anda.
* **`MQTT_PASSWORD`**: *Password* MQTT Anda.
* **`WEBHOOK_URL_WATERLEVEL`**: URL *webhook* Discord untuk pembaruan level air. Untuk mendapatkannya, buka pengaturan saluran server Discord Anda -> Integrasi -> *Webhook* -> *Webhook* Baru.
* **`WEBHOOK_URL_USAGE`**: URL *webhook* Discord untuk pembaruan penggunaan air. (Anda bisa menggunakan *webhook* yang sama atau yang berbeda).
* **`WATERLEVEL_MSG_ID`**: **ID pesan** dari pesan yang sudah ada di saluran Discord Anda yang akan diperbarui oleh *bot* untuk status level air. **Penting**: Anda perlu mengirim pesan *placeholder* di Discord terlebih dahulu, lalu klik kanan pada pesan tersebut (dengan *Developer Mode* diaktifkan di pengaturan Discord) dan salin ID-nya. Ini memungkinkan *bot* untuk terus mengedit pesan yang sama daripada mengirim pesan baru untuk pembaruan level air.

#### 4. Konfigurasi `bot.py`

Buka `bot.py` dan ganti nilai *placeholder*:

* **`dcbottoken`**: Token *bot* Discord Anda. Anda bisa mendapatkannya dari [Discord Developer Portal](https://discord.com/developers/applications) setelah membuat aplikasi dan *bot* Anda.
* **`mongo_client`**: *String* koneksi MongoDB Anda. Ini harus sama dengan `MONGO_URI` di `main.py`.

---

## Menjalankan Sistem

Anda perlu menjalankan `main.py` dan `bot.py` secara bersamaan. Disarankan untuk menjalankannya di sesi terminal terpisah atau menggunakan manajer proses seperti PM2 atau systemd dalam lingkungan produksi.

#### 1. Mulai Penanganan MQTT dan *Webhook* Discord

```bash
python main.py
```

Skrip ini akan terhubung ke broker MQTT Anda, mendengarkan topik `dispenser/level` dan `dispenser/penggunaan`, dan memperbarui Discord serta MongoDB sesuai.

#### 2. Mulai *Bot* Discord

```bash
python bot.py
```

Skrip ini akan menghubungkan *bot* Discord Anda ke server Anda, memungkinkannya untuk merespons perintah.

---

## Perintah *Bot* Discord

Setelah *bot* Anda *online*, Anda dapat menggunakan perintah berikut di server Discord Anda:

* `!status`: Menampilkan jarak sensor ke air yang terakhir direkam, beserta *timestamp*-nya.
* `!grafik`: Menghasilkan dan mengirimkan grafik yang menunjukkan data historis level air (jarak sensor ke air) dari waktu ke waktu.
* `!usagegrafik`: Menghasilkan dan mengirimkan grafik yang menunjukkan data historis penggunaan air (volume dalam ml). Sumbu x merepresentasikan nomor entri data.

---

## Topik dan *Payload* MQTT

Perangkat dispenser Anda harus mempublikasikan data ke topik MQTT berikut dengan *payload* JSON yang ditentukan:

* **Topik Level Air**: `dispenser/level`
    * **Contoh *Payload***:
        ```json
        {"tank_level_cm": 15.2}
        ```
        (Di mana `15.2` adalah jarak dalam sentimeter dari sensor ke permukaan air.)

* **Topik Penggunaan Air**: `dispenser/penggunaan`
    * **Contoh *Payload***:
        ```json
        {"duration_ms": 5000}
        ```
        (Di mana `5000` adalah durasi pengeluaran air dalam milidetik.)

---

## Kustomisasi

* **Fungsi `get_mililiter_from_miliseconds` di `main.py`**: Sesuaikan konstanta `volume_per_milisecond` (`0.04045`) jika dispenser Anda memiliki laju aliran yang berbeda. Nilai ini menunjukkan berapa mililiter air yang dikeluarkan per milidetik.
* ***Embed* Discord**: Anda dapat memodifikasi judul, warna, bidang (*fields*), dan *footer* dari *embed* Discord di `main.py` agar lebih sesuai dengan preferensi Anda.
* **Grafik Matplotlib**: Sesuaikan tampilan grafik (warna, penanda, judul, label) di `bot.py` dengan menyesuaikan parameter `matplotlib.pyplot`.

---

## Pemecahan Masalah

* ***Bot* tidak *online*?** Periksa kembali `dcbottoken` Anda di `bot.py` dan pastikan *bot* Anda memiliki *intent* yang diperlukan di [Discord Developer Portal](https://discord.com/developers/applications) (terutama `Message Content Intent`).
* **Tidak ada data di MongoDB?** Verifikasi `MONGO_URI` Anda di `main.py` dan `bot.py`. Periksa *output* konsol `main.py` untuk kesalahan koneksi atau penyisipan MongoDB.
* **Tidak ada pembaruan Discord?** Pastikan URL `WEBHOOK_URL` Anda benar di `main.py` dan bahwa `WATERLEVEL_MSG_ID` adalah ID pesan yang valid di saluran target. Periksa konsol `main.py` untuk kesalahan *webhook*.
* **Masalah koneksi MQTT?** Verifikasi `MQTT_BROKER`, `MQTT_PORT`, `MQTT_USERNAME`, dan `MQTT_PASSWORD` di `main.py`. Pastikan broker MQTT Anda berjalan dan dapat diakses.

---
---

# Kode Mikrokontroler Dispenser Otomatis (finprosisben.ino)

Kode ini adalah *firmware* untuk mikrokontroler (kemungkinan ESP32) yang berfungsi sebagai otak dari sistem dispenser air otomatis Anda. Kode ini mengelola sensor ultrasonik untuk mendeteksi keberadaan gelas dan level air di tangki, mengontrol pompa air, dan mengirimkan data secara *real-time* ke broker MQTT.

---

## Fungsionalitas Utama

* **Deteksi Gelas:** Menggunakan sensor ultrasonik untuk mendeteksi apakah ada gelas di bawah keran.
* **Pemantauan Level Tangki:** Membaca level air di dalam tangki secara berkala menggunakan sensor ultrasonik lain. Data level tangki dihaluskan menggunakan median filter untuk akurasi yang lebih baik.
* **Kontrol Pompa:** Secara otomatis menyalakan pompa saat gelas terdeteksi dan level air di tangki mencukupi. Pompa akan mati jika gelas diangkat atau level air tangki di bawah ambang batas.
* **Pencatatan Penggunaan Air:** Menghitung durasi pompa menyala dan mengestimasi jumlah air yang digunakan setelah pompa mati.
* **Konektivitas WiFi:** Terhubung ke jaringan WiFi lokal.
* **Konektivitas MQTT:** Mengirimkan data level air tangki dan penggunaan air ke broker MQTT, yang kemudian dapat diakses oleh aplikasi lain (seperti `main.py` dan `bot.py` Anda).
* **Sinkronisasi Waktu:** Menyinkronkan waktu dengan server NTP untuk *timestamp* yang akurat pada data yang dipublikasikan.

---

## Komponen yang Dibutuhkan

* **Mikrokontroler:** ESP32 direkomendasikan karena memiliki WiFi dan SSL.
* **Sensor Ultrasonik (HC-SR04 atau sejenisnya):**
    * Satu untuk deteksi gelas (terhubung ke `TRIG_CUP` dan `ECHO_CUP`).
    * Satu untuk deteksi level tangki (terhubung ke `TRIG_TANK` dan `ECHO_TANK`).
* **Modul Relay:** Untuk mengontrol pompa air (terhubung ke `RELAY_PIN`).
* **Pompa Air:** Pompa DC yang sesuai dengan kebutuhan dispenser Anda.
* **Kabel Jumper dan Breadboard (opsional):** Untuk koneksi.

---

## Konfigurasi Pin

* `TRIG_CUP`: Pin trigger sensor ultrasonik untuk gelas (GPIO 14)
* `ECHO_CUP`: Pin echo sensor ultrasonik untuk gelas (GPIO 15)
* `TRIG_TANK`: Pin trigger sensor ultrasonik untuk tangki (GPIO 5)
* `ECHO_TANK`: Pin echo sensor ultrasonik untuk tangki (GPIO 21)
* `RELAY_PIN`: Pin kontrol modul relay (GPIO 2)

---

## Konstanta dan Ambang Batas Penting

* `CUP_THRESHOLD_CM`: Jarak maksimum (dalam cm) agar gelas dianggap terdeteksi. (Default: 8 cm)
* `TANK_MIN_LEVEL_CM`: Jarak maksimum (dalam cm) dari sensor ke air yang menunjukkan bahwa level air masih cukup. Jika jarak lebih besar dari ini, tangki dianggap kosong atau level rendah. (Default: 13 cm)
* `MAX_MISS_COUNT`: Jumlah pembacaan berturut-turut di mana gelas tidak terdeteksi sebelum pompa dimatikan. (Default: 5)
* `MAX_TANK_MISS`: Jumlah pembacaan berturut-turut di mana sensor tangki gagal sebelum dianggap *timeout*. (Default: 3)
* `SENSOR_TIMEOUT_MS`: Waktu maksimum (dalam milidetik) untuk menunggu respons dari sensor sebelum dianggap gagal. (Default: 2000 ms)
* `TANK_HISTORY_SIZE`: Jumlah pembacaan sensor tangki yang disimpan untuk perhitungan median. (Default: 5)
* `MQTT_PUBLISH_INTERVAL`: Interval waktu (dalam milidetik) untuk mempublikasikan level tangki ke MQTT. (Default: 60000 ms = 1 menit)

---

## Penyiapan

1.  **Instalasi Library:**
    Pastikan Anda telah menginstal *library* berikut di Arduino IDE Anda (melalui `Sketch > Include Library > Manage Libraries...`):
    * `WiFi` (biasanya sudah terinstal)
    * `PubSubClient` oleh Nick O'Leary
    * `ArduinoJson` oleh Benoit Blanchon
    * `WiFiClientSecure` (biasanya sudah terinstal)

2.  **Konfigurasi Kredensial:**
    Ubah nilai *placeholder* berikut dengan kredensial Anda yang sebenarnya:
    * `ssid`: Nama jaringan WiFi Anda.
    * `password`: Kata sandi jaringan WiFi Anda.
    * `mqttServer`: Hostname atau alamat IP broker MQTT Anda.
    * `mqtt_user`: *Username* MQTT Anda.
    * `mqtt_pass`: *Password* MQTT Anda.

3.  **Unggah Kode:**
    Pilih jenis board ESP32 Anda di Arduino IDE (`Tools > Board`) dan *port* serial yang benar (`Tools > Port`), lalu unggah kode ini ke mikrokontroler Anda.

---

## Cara Kerja

1.  **`setup()` Function:**
    * Menginisialisasi komunikasi serial.
    * Mengatur pin sensor dan relay.
    * Menghubungkan ke WiFi menggunakan `setupWiFi()`.
    * Menyinkronkan waktu dengan server NTP menggunakan `setupTime()` (penting untuk *timestamp* MQTT).
    * Mengatur klien MQTT dengan mode *insecure* (untuk SSL tanpa validasi sertifikat, **tidak direkomendasikan untuk produksi tanpa pemahaman risiko**).

2.  **`loop()` Function:**
    * **Membaca Sensor:** Secara berulang membaca jarak dari kedua sensor ultrasonik.
    * **Penanganan *Timeout* Sensor:** Jika sensor tidak memberikan pembacaan yang valid dalam `SENSOR_TIMEOUT_MS`, nilai terakhir yang valid direset, menandakan kemungkinan masalah sensor.
    * **Median Filter (Tangki):** Pembacaan sensor tangki disimpan dalam *array* dan nilai median digunakan untuk menghaluskan data, mengurangi *noise*.
    * **Log Serial:** Mencetak jarak sensor gelas dan tangki ke *Serial Monitor* jika ada perubahan yang signifikan.
    * **Logika Kontrol Pompa:**
        * Mengecek `isWaterSufficient`: Jika level air tangki di bawah ambang batas (`TANK_MIN_LEVEL_CM`), pompa akan mati jika sedang menyala.
        * Mengecek `isCupDetected`: Jika gelas terdeteksi (`lastValidCup` di bawah `CUP_THRESHOLD_CM`) DAN air cukup:
            * Pompa akan menyala jika belum menyala.
            * `tankLevelBeforePump` dan `pumpStartTime` dicatat untuk perhitungan penggunaan air nanti.
        * Jika gelas tidak terdeteksi (dan pompa sedang menyala):
            * `missCount` bertambah. Jika mencapai `MAX_MISS_COUNT`, pompa dimatikan (menandakan gelas telah diangkat).
            * Fungsi `printWaterUsedAndTime()` dipanggil untuk mencatat penggunaan.
    * **Konektivitas MQTT:**
        * Memastikan koneksi MQTT tetap terhubung menggunakan `reconnectMQTT()`.
        * Menjalankan *loop* klien MQTT untuk memproses pesan.
        * Mempublikasikan level tangki ke topik `dispenser/level` setiap `MQTT_PUBLISH_INTERVAL`.

3.  **Fungsi *Helper*:**
    * `readDistanceCM(trigPin, echoPin)`: Mengukur jarak menggunakan sensor ultrasonik.
    * `getMedianTank()`: Menghitung nilai median dari histori pembacaan level tangki.
    * `printWaterUsedAndTime()`: Menghitung dan mencetak perkiraan volume air yang digunakan dan durasi pompa menyala, lalu mempublikasikan data penggunaan ke MQTT.
    * `setupTime()`: Mengatur waktu menggunakan NTP.
    * `setupWiFi()`: Menghubungkan ke WiFi.
    * `reconnectMQTT()`: Menghubungkan ulang ke broker MQTT jika koneksi terputus.
    * `publishTankLevelMQTT()`: Mempublikasikan data level tangki ke topik MQTT.
    * `publishWaterUsageMQTT(duration)`: Mempublikasikan data durasi penggunaan air ke topik MQTT.

---

## Penting untuk Diperhatikan

* **Keamanan MQTT (`secureClient.setInsecure()`):** Penggunaan `secureClient.setInsecure()` menonaktifkan validasi sertifikat SSL/TLS. Meskipun ini memudahkan koneksi awal, **tidak disarankan untuk lingkungan produksi** karena dapat membuat komunikasi rentan terhadap serangan *man-in-the-middle*. Untuk keamanan yang lebih baik, Anda harus mengimplementasikan validasi sertifikat yang tepat.
* **Kalibrasi Sensor:** Nilai `CUP_THRESHOLD_CM` dan `TANK_MIN_LEVEL_CM` mungkin perlu disesuaikan berdasarkan penempatan fisik sensor dan dimensi dispenser Anda. Lakukan kalibrasi dengan menguji berbagai level dan posisi gelas.
* **Akurasi Penggunaan Air:** Perkiraan penggunaan air berdasarkan durasi pompa menyala adalah estimasi. Untuk akurasi yang lebih tinggi, Anda mungkin memerlukan sensor aliran air.
* **Interval Publikasi MQTT:** `MQTT_PUBLISH_INTERVAL` diatur ke 1 menit. Anda dapat menyesuaikannya sesuai kebutuhan pembaruan data Anda. Interval yang terlalu sering dapat membebani broker MQTT, sementara yang terlalu jarang dapat menyebabkan data kurang *real-time*.

---