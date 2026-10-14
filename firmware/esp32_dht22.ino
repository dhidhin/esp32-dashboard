/*
  ==============================================================
  ESP32 + DHT22/DHT11 ke Supabase IoT Monitoring
  ==============================================================
  Program ini membaca data sensor DHT22 (Suhu & Kelembapan)
  dan mengirimkannya langsung ke REST API Supabase via HTTPS POST.

  Kebutuhan Library (Arduino IDE Library Manager):
  - DHT sensor library by Adafruit
  - Adafruit Unified Sensor
  - ArduinoJson (opsional jika ingin parsing JSON lanjut)

  Pinout DHT22 ke ESP32:
  - VCC   -> 3.3V / 5V
  - DATA  -> GPIO 4 (Bisa disesuaikan, tambahkan pull-up resistor 10k ke VCC jika modul belum ada)
  - GND   -> GND
  ==============================================================
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <DHT.h>

// ================= KREDENSIAL WIFI =================
const char* WIFI_SSID     = "NAMA_WIFI_ANDA";
const char* WIFI_PASSWORD = "PASSWORD_WIFI_ANDA";

// ================= KREDENSIAL SUPABASE =============
// Contoh: "https://xyzcompany.supabase.co"
const char* SUPABASE_URL      = "https://YOUR_PROJECT_ID.supabase.co";
// Masukkan publishable anon key Supabase Anda
const char* SUPABASE_ANON_KEY = "YOUR_SUPABASE_ANON_KEY";

// Endpoint tabel Supabase
const String SUPABASE_ENDPOINT = String(SUPABASE_URL) + "/rest/v1/sensor_readings";

// ================= KONFIGURASI SENSOR ==============
#define DHTPIN 4          // GPIO yang terhubung ke pin DATA DHT
#define DHTTYPE DHT22     // Ganti dengan DHT11 jika menggunakan sensor DHT11

DHT dht(DHTPIN, DHTTYPE);

// Interval pengiriman data (dalam milidetik)
const unsigned long SEND_INTERVAL = 5000; // Kirim tiap 5 detik
unsigned long lastSendTime = 0;

void connectWiFi() {
  Serial.print("Menghubungkan ke WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Berhasil Terhubung!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void sendDataToSupabase(float suhu, float kelembapan) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Koneksi WiFi terputus! Mencoba menghubungkan kembali...");
    connectWiFi();
    return;
  }

  WiFiClientSecure client;
  // Untuk kemudahan koneksi SSL tanpa load Root CA cert manual
  client.setInsecure();

  HTTPClient http;

  Serial.println("\nMengirim data ke Supabase...");
  if (http.begin(client, SUPABASE_ENDPOINT)) {
    // Header HTTP yang wajib untuk Supabase REST API
    http.addHeader("Content-Type", "application/json");
    http.addHeader("apikey", SUPABASE_ANON_KEY);
    http.addHeader("Authorization", String("Bearer ") + SUPABASE_ANON_KEY);
    http.addHeader("Prefer", "return=minimal");

    // Format Payload JSON
    String requestBody = "{\"suhu\":" + String(suhu, 2) + ",\"kelembapan\":" + String(kelembapan, 2) + "}";
    Serial.print("Payload: ");
    Serial.println(requestBody);

    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode == 201 || httpResponseCode == 200) {
      Serial.print("Berhasil terkirim! Status Code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Gagal mengirim! HTTP Status Code: ");
      Serial.println(httpResponseCode);
      String response = http.getString();
      Serial.println("Response dari server: " + response);
    }

    http.end();
  } else {
    Serial.println("Gagal menginisialisasi koneksi HTTPS");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n====================================");
  Serial.println("  ESP32 DHT22 - Supabase Dashboard   ");
  Serial.println("====================================");

  // Inisialisasi sensor DHT
  dht.begin();

  // Koneksikan ke WiFi
  connectWiFi();
}

void loop() {
  unsigned long currentMillis = millis();

  // Periksa apakah waktu pengiriman sudah tercapai
  if (currentMillis - lastSendTime >= SEND_INTERVAL) {
    lastSendTime = currentMillis;

    // Baca kelembapan dan suhu
    float kelembapan = dht.readHumidity();
    float suhu = dht.readTemperature();

    // Validasi pembacaan sensor
    if (isnan(kelembapan) || isnan(suhu)) {
      Serial.println("Peringatan: Gagal membaca data dari sensor DHT22!");
      return;
    }

    Serial.println("------------------------------------");
    Serial.printf("Pembacaan Sensor -> Suhu: %.2f °C | Kelembapan: %.2f %%\n", suhu, kelembapan);

    // Kirim data ke Supabase
    sendDataToSupabase(suhu, kelembapan);
  }
}
