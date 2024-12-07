// Definisi Template Blynk
#define BLYNK_TEMPLATE_ID "TMPL6HOsfa_3b"
#define BLYNK_TEMPLATE_NAME "Unity Gas Monitoring"
#define BLYNK_AUTH_TOKEN "MA5yxkC-Afvj25fdhqTJfNKe_9hQEdcJ"


// Library yang dibutuhkan
#include <WiFi.h>
#include <BlynkSimpleEsp32.h> // Blynk library sudah termasuk BlynkTimer
#include <math.h> // Untuk fungsi matematika seperti pow()

// Definisi PIN
#define MQ135_PIN 34
#define LED_PIN 35
#define BUZZER_PIN 33
#define SR04_TRIG_PIN 25
#define SR04_ECHO_PIN 26

// Definisi Virtual Pins untuk MQ-135
#define VIRTUAL_CO2_PIN V1
#define VIRTUAL_CO_PIN V2
#define VIRTUAL_CH4_PIN V3
#define VIRTUAL_NH3_PIN V4

#define VIRTUAL_THRESHOLD_CO2 V10
#define VIRTUAL_THRESHOLD_CO V11
#define VIRTUAL_THRESHOLD_CH4 V12
#define VIRTUAL_THRESHOLD_NH3 V13

// Definisi Virtual Pins untuk SR04
#define VIRTUAL_DISTANCE_PIN V5
#define VIRTUAL_MESSAGE_PIN V6
#define VIRTUAL_SLEEP_COUNTDOWN V7
#define VIRTUAL_SET_SLEEP_TIME V8
#define VIRTUAL_SET_SAFE_DISTANCE V9

// Parameter untuk Sensor SR04
#define SOUND_SPEED 0.034 // cm/uS
#define CM_TO_INCH 0.393701

// Kredensial Wi-Fi
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "HNIS";       // Ganti dengan SSID Wi-Fi Anda
char pass[] = "qwerty90";      // Ganti dengan Password Wi-Fi Anda

// Variabel untuk SR04
long duration;
float distanceCm;
float distanceInch;

// Parameter Deep Sleep
int sleep_time = 30; // Default sleep time dalam detik

// Parameter Keamanan Jarak
int safe_distance = 20; // Default jarak aman dalam cm

// Ambang batas gas (ppm) - Default
float threshold_co2 = 1000.0;   // Contoh: 1000 ppm CO2
float threshold_co = 50.0;       // Contoh: 50 ppm CO
float threshold_ch4 = 100.0;     // Contoh: 100 ppm CH4
float threshold_nh3 = 25.0;      // Contoh: 25 ppm NH3

BlynkTimer timer;

void setup() {
  // Inisialisasi Serial
  Serial.begin(115200);

  // Inisialisasi PIN
  pinMode(MQ135_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(SR04_TRIG_PIN, OUTPUT);
  pinMode(SR04_ECHO_PIN, INPUT);

  // Matikan LED dan Buzzer pada awalnya
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Koneksi ke Wi-Fi
  Serial.println("Menghubungkan ke Wi-Fi...");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nTerhubung ke Wi-Fi");

  // Tes DNS Resolution untuk Blynk
  IPAddress ip;
  if (WiFi.hostByName("blynk-cloud.com", ip)) {
    Serial.println("Blynk IP Terhubung: " + ip.toString());
  } else {
    Serial.println("DNS resolution gagal");
  }

  // Koneksi ke Blynk
  Serial.println("Menghubungkan ke Blynk...");
  Blynk.begin(auth, ssid, pass);

  if (Blynk.connected()) {
    Serial.println("Terhubung ke Blynk");
  } else {
    Serial.println("Gagal terhubung ke Blynk");
  }

  // Menampilkan alasan wakeup setelah deep sleep
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
    Serial.println("Bangun dari deep sleep");
    Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Bangun dari deep sleep");
  } else {
    Serial.println("Power-on atau reset");
  }

  // Set interval untuk membaca sensor setiap 1 detik
  timer.setInterval(1000L, readSensors);
}

void loop() {
  Blynk.run();
  timer.run();
}

// Fungsi untuk membaca sensor MQ-135 dan SR04
void readSensors() {
  readMQ135();
  readSR04();
  checkThresholds();
}

// Fungsi untuk membaca sensor MQ-135
void readMQ135() {
  // Membaca nilai dari sensor MQ-135
  int rawValue = analogRead(MQ135_PIN);  // Membaca nilai analog dari sensor
  float voltage = rawValue / 4095.0 * 3.3;  // ESP32 ADC 12-bit dan 3.3V reference
  float resistance = (3.3 - voltage) / voltage;  // Menghitung resistansi sensor

  // Menghitung konsentrasi gas (ppm) berdasarkan resistansi
  float co2_ppm = 1.0 / (0.03588 * pow(resistance, 1.336));  
  float co_ppm = co2_ppm / 2.2;        
  float methane_ppm = co2_ppm / 2.7;   
  float ammonia_ppm = co2_ppm / 3.6;   

  // Menampilkan hasil pembacaan sensor di Serial Monitor
  Serial.print("MQ-135 - Raw Analog Value: ");
  Serial.println(rawValue);
  Serial.print("Voltage: ");
  Serial.println(voltage, 2);    // Menampilkan tegangan dengan 2 digit desimal
  Serial.print("Sensor Resistance: ");
  Serial.println(resistance, 2); 
  
  // Menampilkan nilai ppm untuk CO2, CO, NH3, dan CH4
  Serial.print("CO2 PPM: ");
  Serial.println(co2_ppm, 2);       
  
  Serial.print("CO PPM: ");
  Serial.println(co_ppm, 2);         
  
  Serial.print("Methane (CH4) PPM: ");
  Serial.println(methane_ppm, 2);    
  
  Serial.print("Ammonia (NH3) PPM: ");
  Serial.println(ammonia_ppm, 2);    

  // Mengirim data ke Blynk melalui Virtual Pins
  Blynk.virtualWrite(VIRTUAL_CO2_PIN, co2_ppm);  
  Blynk.virtualWrite(VIRTUAL_CO_PIN, co_ppm);     
  Blynk.virtualWrite(VIRTUAL_CH4_PIN, methane_ppm); 
  Blynk.virtualWrite(VIRTUAL_NH3_PIN, ammonia_ppm);  
}

// Fungsi untuk membaca sensor SR04
void readSR04() {
  // Bersihkan trigPin
  digitalWrite(SR04_TRIG_PIN, LOW);
  delayMicroseconds(2);

  // Set trigPin HIGH selama 10 mikrodetik
  digitalWrite(SR04_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(SR04_TRIG_PIN, LOW);

  // Hitung jarak berdasarkan waktu pantulan
  duration = pulseIn(SR04_ECHO_PIN, HIGH);
  distanceCm = duration * SOUND_SPEED / 2;
  distanceInch = distanceCm * CM_TO_INCH;

  // Kirim data jarak ke Blynk (Virtual Pin V5)
  Blynk.virtualWrite(VIRTUAL_DISTANCE_PIN, distanceCm);
  Serial.print("SR04 - Distance (cm): ");
  Serial.println(distanceCm);

  // Logika berdasarkan jarak
  if (distanceCm > safe_distance && distanceCm < (safe_distance + 10)){
    Serial.println("Peringatan: Pegawai mendekat ke mesin!");
    Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Peringatan: Pegawai mendekat ke mesin!");
    // Nyalakan LED dan Buzzer sebagai peringatan
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
  }
  else if (distanceCm <= safe_distance) {
    Serial.println("Aksi: Mematikan mesin untuk mencegah kecelakaan.");
    Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Mematikan mesin untuk mencegah kecelakaan.");
    // Matikan LED dan Buzzer
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    // Masuk ke mode deep sleep
    enterDeepSleep();
  }
  else {
    // Jarak aman, matikan LED dan Buzzer jika menyala
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Pegawai dalam jarak aman.");
  }
}

// Fungsi untuk mengecek ambang batas gas
void checkThresholds() {
  bool gasExceeded = false;
  String warningMessage = "";

  // Cek setiap gas terhadap ambang batasnya
  if (threshold_co2 > 0 && analogRead(MQ135_PIN) > threshold_co2) {
    warningMessage += "Peringatan: Kadar CO2 melebihi batas aman!\n";
    gasExceeded = true;
  }
  if (threshold_co > 0 && analogRead(MQ135_PIN) > threshold_co) {
    warningMessage += "Peringatan: Kadar CO melebihi batas aman!\n";
    gasExceeded = true;
  }
  if (threshold_ch4 > 0 && analogRead(MQ135_PIN) > threshold_ch4) {
    warningMessage += "Peringatan: Kadar Methane (CH4) melebihi batas aman!\n";
    gasExceeded = true;
  }
  if (threshold_nh3 > 0 && analogRead(MQ135_PIN) > threshold_nh3) {
    warningMessage += "Peringatan: Kadar Ammonia (NH3) melebihi batas aman!\n";
    gasExceeded = true;
  }

  if (gasExceeded) {
    Serial.println(warningMessage);
    Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, warningMessage);
    // Aksi: Mematikan mesin melalui deep sleep
    enterDeepSleep();
  }
}

// Fungsi untuk memasuki mode deep sleep
void enterDeepSleep() {
  // Jalankan timer countdown
  for (int remainingTime = sleep_time; remainingTime >= 0; remainingTime--) {
    Blynk.virtualWrite(VIRTUAL_SLEEP_COUNTDOWN, remainingTime); // Kirim waktu tersisa ke Virtual Pin V7
    Serial.print("Waktu tersisa sebelum deep sleep: ");
    Serial.println(remainingTime);

    delay(1000); // Tunggu 1 detik sebelum memperbarui waktu
  }

  // Masuk ke mode deep sleep setelah timer selesai
  Serial.println("Memasuki deep sleep...");
  Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Memasuki deep sleep...");
  esp_sleep_enable_timer_wakeup(sleep_time * 1000000); // Set deep sleep timer
  esp_deep_sleep_start(); // Masuk ke deep sleep
}

// Fungsi untuk menerima pembaruan sleep_time dari Blynk
BLYNK_WRITE(VIRTUAL_SET_SLEEP_TIME) {
  int new_sleep_time = param.asInt();
  if (new_sleep_time > 0) {
    sleep_time = new_sleep_time;
    Serial.print("Sleep time diupdate menjadi: ");
    Serial.println(sleep_time);
    Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Sleep time diupdate menjadi: " + String(sleep_time) + " detik");
  } else {
    Serial.println("Sleep time tidak valid");
    Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Sleep time tidak valid");
  }
}

// Fungsi untuk menerima pembaruan safe_distance dari Blynk
BLYNK_WRITE(VIRTUAL_SET_SAFE_DISTANCE) {
  int new_safe_distance = param.asInt();
  if (new_safe_distance > 0) {
    safe_distance = new_safe_distance;
    Serial.print("Safe distance diupdate menjadi: ");
    Serial.println(safe_distance);
    Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Safe distance diupdate menjadi: " + String(safe_distance) + " cm");
  } else {
    Serial.println("Safe distance tidak valid");
    Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Safe distance tidak valid");
  }
}

// Fungsi untuk menerima pembaruan ambang batas CO2 dari Blynk
BLYNK_WRITE(VIRTUAL_THRESHOLD_CO2) {
  threshold_co2 = param.asFloat();
  Serial.print("Ambang batas CO2 diubah menjadi: ");
  Serial.println(threshold_co2);
  Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Ambang batas CO2 diubah menjadi: " + String(threshold_co2) + " ppm");
}

// Fungsi untuk menerima pembaruan ambang batas CO dari Blynk
BLYNK_WRITE(VIRTUAL_THRESHOLD_CO) {
  threshold_co = param.asFloat();
  Serial.print("Ambang batas CO diubah menjadi: ");
  Serial.println(threshold_co);
  Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Ambang batas CO diubah menjadi: " + String(threshold_co) + " ppm");
}

// Fungsi untuk menerima pembaruan ambang batas CH4 dari Blynk
BLYNK_WRITE(VIRTUAL_THRESHOLD_CH4) {
  threshold_ch4 = param.asFloat();
  Serial.print("Ambang batas CH4 diubah menjadi: ");
  Serial.println(threshold_ch4);
  Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Ambang batas CH4 diubah menjadi: " + String(threshold_ch4) + " ppm");
}

// Fungsi untuk menerima pembaruan ambang batas NH3 dari Blynk
BLYNK_WRITE(VIRTUAL_THRESHOLD_NH3) {
  threshold_nh3 = param.asFloat();
  Serial.print("Ambang batas NH3 diubah menjadi: ");
  Serial.println(threshold_nh3);
  Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Ambang batas NH3 diubah menjadi: " + String(threshold_nh3) + " ppm");
}
