/*
  ==============================================================
  ESP32 + DHT22 + LCD I2C + Buzzer ke Supabase IoT Monitoring
  ==============================================================
  Author   : Dhidhin Haris (XII RPL)
  Simulasi : https://wokwi.com/projects/477206242817367041
  Web Demo : https://esp32-dashboard-nu.vercel.app/

  Fitur Firmware:
  - Membaca sensor suhu & kelembapan DHT22 secara berkala (tiap 5 detik)
  - Menampilkan suhu, kelembapan, & status alarm ke LCD 16x2 I2C
  - Membunyikan Buzzer alarm otomatis jika suhu mencapai >= 50 °C (Overheat)
  - Mengirim payload JSON ke Supabase REST API via HTTPS POST
  - Auto-drift acak saat simulasi Wokwi dalam mode idle
  ==============================================================
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// ========================================
// KONFIGURASI WIFI & SUPABASE
// ========================================
// Pada simulator Wokwi gunakan "Wokwi-GUEST", untuk hardware nyata gunakan SSID WiFi Anda
const char* WIFI_SSID     = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// Ganti dengan URL dan Anon Key project Supabase Anda
const char* SUPABASE_URL     = "https://YOUR_PROJECT_ID.supabase.co";
const char* SUPABASE_API_KEY = "YOUR_SUPABASE_ANON_KEY";
const char* SUPABASE_TABLE   = "/rest/v1/sensor_readings";

// ========================================
// DEFINISI PIN & PERANGKAT
// ========================================
#define DHT_PIN    13     // Pin Data sensor DHT22
#define DHT_TYPE   DHT22  // Tipe sensor DHT22 (AM2302)
#define BUZZER_PIN 14     // Pin Buzzer alarm overheat

DHT dht(DHT_PIN, DHT_TYPE);

#define LCD_ADDRESS 0x27  // Alamat I2C LCD
#define LCD_COLUMNS 16
#define LCD_ROWS    2

LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

// ========================================
// TIMING & AMBANG BATAS
// ========================================
unsigned long previousMillis = 0;
const unsigned long interval = 5000; // Interval pengiriman ke Supabase (5 detik)

// Batas ambang alarm suhu overheat (°C)
const float TEMP_ALARM_THRESHOLD = 50.0;

// Pelacak status sensor untuk simulasi Wokwi
float lastRawTemp = -999.0;
float lastRawHum  = -999.0;
unsigned long lastChangeTime = 0;

float currentTemp = 25.0;
float currentHum  = 50.0;

void connectWiFi() {
  Serial.println();
  Serial.print("Menghubungkan ke WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Berhasil Terhubung!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());

  delay(2000);
}

void sendToSupabase(float temperature, float humidity) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Koneksi WiFi terputus! Mencoba rekoneksi...");
    connectWiFi();
    return;
  }

  WiFiClientSecure client;
  client.setInsecure(); // Bypass verifikasi sertifikat SSL

  HTTPClient http;
  String url = String(SUPABASE_URL) + SUPABASE_TABLE;

  Serial.println("\nMengirim data ke Supabase...");
  if (!http.begin(client, url)) {
    Serial.println("Gagal membuka koneksi HTTPS!");
    return;
  }

  // Header wajib Supabase REST API
  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", SUPABASE_API_KEY);
  http.addHeader("Authorization", "Bearer " + String(SUPABASE_API_KEY));
  http.addHeader("Prefer", "return=minimal");

  // Format payload JSON: {"suhu": 28.50, "kelembapan": 65.00}
  String jsonData = "{\"suhu\":" + String(temperature, 2) + ",\"kelembapan\":" + String(humidity, 2) + "}";
  Serial.println("Payload: " + jsonData);

  int httpResponseCode = http.POST(jsonData);

  if (httpResponseCode == 201 || httpResponseCode == 200) {
    Serial.printf("Berhasil terkirim! HTTP Status: %d\n", httpResponseCode);
  } else {
    Serial.printf("Gagal mengirim data! HTTP Status: %d\n", httpResponseCode);
    String response = http.getString();
    Serial.println("Response server: " + response);
  }

  http.end();
}

void setup() {
  Serial.begin(115200);

  // Inisialisasi Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);

  // Inisialisasi I2C (SDA=21, SCL=22)
  Wire.begin(21, 22);

  // Inisialisasi LCD 1602
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("ESP32 IoT");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  delay(2000);

  // Inisialisasi sensor DHT
  dht.begin();
  randomSeed(esp_random());

  connectWiFi();
  lastChangeTime = millis();
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Baca sensor DHT22
    float rawHum = dht.readHumidity();
    float rawTemp = dht.readTemperature();

    if (isnan(rawHum) || isnan(rawTemp)) {
      Serial.println("Gagal membaca sensor DHT22!");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("DHT22 ERROR");
      lcd.setCursor(0, 1);
      lcd.print("Cek sensor");
      noTone(BUZZER_PIN);
      return;
    }

    // Inisialisasi awal nilai
    if (lastRawTemp == -999.0) {
      lastRawTemp = rawTemp;
      lastRawHum = rawHum;
      currentTemp = rawTemp;
      currentHum = rawHum;
      lastChangeTime = currentMillis;
    }

    // Deteksi jika pengguna menggeser slider DHT22 di simulator Wokwi
    if (fabs(rawTemp - lastRawTemp) > 0.05 || fabs(rawHum - lastRawHum) > 0.05) {
      lastRawTemp = rawTemp;
      lastRawHum = rawHum;
      currentTemp = rawTemp;
      currentHum = rawHum;
      lastChangeTime = currentMillis;
      Serial.println("[Manual] Nilai diubah via slider Wokwi.");
    } else {
      // Jika tidak ada interaksi selama >= 10 detik, buat fluktuasi halus otomatis
      if (currentMillis - lastChangeTime >= 10000) {
        float deltaTemp = random(-3, 4);
        currentTemp += deltaTemp;

        float deltaHum = (-deltaTemp * 1.5) + random(-5, 6);
        currentHum += deltaHum;

        currentTemp = constrain(currentTemp, 15.0, 80.0);
        currentHum  = constrain(currentHum, 20.0, 95.0);

        Serial.println("[Auto-Drift] Nilai berfluktuasi otomatis.");
      }
    }

    // ========================================
    // LOGIKA ALARM OVERHEAT (SUHU >= 50 °C)
    // ========================================
    bool isOverheat = (currentTemp >= TEMP_ALARM_THRESHOLD);

    if (isOverheat) {
      tone(BUZZER_PIN, 2000); // Bunyikan alarm 2000 Hz
      Serial.println(">>> [ALARM OVERHEAT AKTIF] Suhu >= 50°C! <<<");
    } else {
      noTone(BUZZER_PIN);
    }

    // Output ke Serial Monitor
    Serial.println("====================================");
    Serial.printf("Suhu       : %.1f °C %s\n", currentTemp, isOverheat ? "[OVERHEAT!]" : "");
    Serial.printf("Kelembapan : %.1f %%\n", currentHum);

    // Update tampilan pada LCD 16x2
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Suhu: ");
    lcd.print(currentTemp, 1);
    lcd.print((char)223); // Simbol derajat °
    lcd.print("C");
    if (isOverheat) {
      lcd.setCursor(12, 0);
      lcd.print("!ALR");
    }

    lcd.setCursor(0, 1);
    lcd.print("Hum : ");
    lcd.print(currentHum, 1);
    lcd.print("%");

    // Kirim data ke Supabase
    sendToSupabase(currentTemp, currentHum);
  }
}
