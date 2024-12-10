// Definisi Template Blynk
#define BLYNK_TEMPLATE_ID "TMPL6A86Zmqw2"
#define BLYNK_TEMPLATE_NAME "Proyek IOT"
#define BLYNK_AUTH_TOKEN "U9VmCOAObBfetnceDNMd7s3aWXo8BN27"

// Library yang dibutuhkan
#include <WiFi.h>
#include <BlynkSimpleEsp32.h> // Blynk library sudah termasuk BlynkTimer
#include <math.h> // Untuk fungsi matematika seperti pow()
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h> // Tambahkan library queue

// Definisi PIN
#define MQ135_PIN 34
#define LED_PIN 14
#define BUZZER_PIN 12
#define SR04_TRIG_PIN 26
#define SR04_ECHO_PIN 27

// Definisi Virtual Pins untuk MQ-135
#define VIRTUAL_CO2_PIN V6
#define VIRTUAL_CO_PIN V7
#define VIRTUAL_CH4_PIN V8
#define VIRTUAL_NH3_PIN V9

#define VIRTUAL_THRESHOLD_CO2 V10
#define VIRTUAL_THRESHOLD_CO V11
#define VIRTUAL_THRESHOLD_CH4 V12
#define VIRTUAL_THRESHOLD_NH3 V13

// Definisi Virtual Pins untuk SR04
#define VIRTUAL_DISTANCE_PIN V1
#define VIRTUAL_MESSAGE_PIN V2
#define VIRTUAL_SLEEP_COUNTDOWN V3
#define VIRTUAL_SET_SLEEP_TIME V4
#define VIRTUAL_SET_SAFE_DISTANCE V5

// Parameter untuk Sensor SR04
#define SOUND_SPEED 0.034 // cm/uS
#define CM_TO_INCH 0.393701

// Kredensial Wi-Fi
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "TrioHa 5G";  // Ganti dengan SSID Wi-Fi Anda
char pass[] = "Sanjaya24";  // Ganti dengan Password Wi-Fi Anda

// Struktur untuk menyimpan data sensor
typedef struct {
  float co2_ppm;
  float co_ppm;
  float methane_ppm;
  float ammonia_ppm;
  float distanceCm;
} SensorData_t;

// Variabel untuk SR04
long duration;
float distanceCm;
float distanceInch;

float co2_ppm;
float co_ppm;
float methane_ppm;
float ammonia_ppm;

// Parameter Deep Sleep
int sleep_time = 10; // Default sleep time dalam detik

// Parameter Keamanan Jarak
int safe_distance = 20; // Default jarak aman dalam cm

// Ambang batas gas (ppm) - Default
float threshold_co2 = 1000.0;   // Contoh: 1000 ppm CO2
float threshold_co = 50.0;       // Contoh: 50 ppm CO
float threshold_ch4 = 100.0;     // Contoh: 100 ppm CH4
float threshold_nh3 = 25.0;      // Contoh: 25 ppm NH3

BlynkTimer timer;

// Deklarasi Queue Handle
QueueHandle_t xSensorQueue;

// Deklarasi Semaphore Handle
SemaphoreHandle_t xSemaphoreDataReady;

// Fungsi untuk memasuki mode deep sleep
void enterDeepSleep() {
  // Jalankan timer countdown
  for (int remainingTime = sleep_time; remainingTime >= 0; remainingTime--) {
    Blynk.virtualWrite(VIRTUAL_SLEEP_COUNTDOWN, remainingTime); // Kirim waktu tersisa ke Virtual Pin V3
    Serial.print("Waktu deep sleep tersisa: ");
    Serial.println(remainingTime);

    vTaskDelay(pdMS_TO_TICKS(1000)); // Tunggu 1 detik sebelum memperbarui waktu
  }

  // Masuk ke mode deep sleep setelah timer selesai
  Serial.println("Memasuki deep sleep...");
  Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Memasuki deep sleep...");
  esp_sleep_enable_timer_wakeup(sleep_time * 1000000); // Set deep sleep timer
  esp_deep_sleep_start(); // Masuk ke deep sleep
}

// Fungsi untuk membaca sensor MQ-135
void readMQ135() {
  // Membaca nilai dari sensor MQ-135
  int rawValue = analogRead(MQ135_PIN);  // Membaca nilai analog dari sensor
  float voltage = rawValue / 4095.0 * 3.3;  // ESP32 ADC 12-bit dan 3.3V reference
  float resistance = (3.3 - voltage) / voltage;  // Menghitung resistansi sensor

  // Menghitung konsentrasi gas (ppm) berdasarkan resistansi
  co2_ppm = 1.0 / (0.03588 * pow(resistance, 1.336));  
  co_ppm = co2_ppm / 2.2;        
  methane_ppm = co2_ppm / 2.7;   
  ammonia_ppm = co2_ppm / 3.6;   

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
}

// Fungsi untuk membaca sensor SR04
void readSR04() {
  static unsigned long lastTime = 0;
  unsigned long currentTime = millis();

  // Jalankan setiap 1 detik
  if (currentTime - lastTime >= 1000) {
    lastTime = currentTime;
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

    // Menampilkan hasil pembacaan sensor di Serial Monitor
    Serial.print("SR04 - Distance (cm): ");
    Serial.println(distanceCm);

    // Logika berdasarkan jarak
    if (distanceCm > safe_distance && distanceCm < (safe_distance + 10)){
      Serial.println("Peringatan: Pegawai mendekat ke mesin!");
      // Nyalakan LED dan Buzzer sebagai peringatan
      digitalWrite(LED_PIN, HIGH);
      digitalWrite(BUZZER_PIN, HIGH);
    }
    else if (distanceCm <= safe_distance) {
      Serial.println("Aksi: Mematikan mesin untuk mencegah kecelakaan.");
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
      Serial.println("Pegawai dalam jarak aman.");
    }
  }
}

// Fungsi untuk membaca sensor MQ-135 dan SR04
void readSensors(void *pvParameters) {
  SensorData_t sensorData;
  
  for(;;){
    readMQ135();
    readSR04();

    // Mengemas data sensor ke dalam struct
    sensorData.co2_ppm = co2_ppm;
    sensorData.co_ppm = co_ppm;
    sensorData.methane_ppm = methane_ppm;
    sensorData.ammonia_ppm = ammonia_ppm;
    sensorData.distanceCm = distanceCm;

    // Mengirim data ke queue
    if (xQueueSend(xSensorQueue, &sensorData, portMAX_DELAY) != pdPASS) {
      Serial.println("Gagal mengirim data ke queue!");
    }

    // Memberi sinyal bahwa data sensor telah diperbarui
    xSemaphoreGive(xSemaphoreDataReady);
    
    // Delay non-blocking selama 1 detik
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// Fungsi blynkTask menunggu sinyal sebelum menjalankan Blynk.run()
void blynkTask(void *pvParameters) {
  SensorData_t receivedData;

  for (;;) {
    // Tunggu hingga semaphore tersedia
    if (xSemaphoreTake(xSemaphoreDataReady, portMAX_DELAY)) {
      // Menerima data dari queue dan memprosesnya
      if (xQueueReceive(xSensorQueue, &receivedData, 0) == pdPASS) {
        // Mengirim data ke Blynk
        Blynk.virtualWrite(VIRTUAL_CO2_PIN, receivedData.co2_ppm);  
        Blynk.virtualWrite(VIRTUAL_CO_PIN, receivedData.co_ppm);     
        Blynk.virtualWrite(VIRTUAL_CH4_PIN, receivedData.methane_ppm); 
        Blynk.virtualWrite(VIRTUAL_NH3_PIN, receivedData.ammonia_ppm);  
        Blynk.virtualWrite(VIRTUAL_DISTANCE_PIN, receivedData.distanceCm);
        
        // Cek ambang batas gas dan jarak
        bool gasExceeded = false;
        String warningMessage = "";

        if (receivedData.co2_ppm > threshold_co2) {
          warningMessage += "Peringatan: Kadar CO2 melebihi batas aman!\n";
          gasExceeded = true;
        }
        if (receivedData.co_ppm > threshold_co) {
          warningMessage += "Peringatan: Kadar CO melebihi batas aman!\n";
          gasExceeded = true;
        }
        if (receivedData.methane_ppm > threshold_ch4) {
          warningMessage += "Peringatan: Kadar Methane (CH4) melebihi batas aman!\n";
          gasExceeded = true;
        }
        if (receivedData.ammonia_ppm > threshold_nh3) {
          warningMessage += "Peringatan: Kadar Ammonia (NH3) melebihi batas aman!\n";
          gasExceeded = true;
        }

        if (receivedData.distanceCm <= safe_distance) {
          warningMessage += "Peringatan: Pegawai mendekat ke mesin!\n";
          gasExceeded = true;
        }

        if (gasExceeded) {
          Serial.println(warningMessage);
          Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, warningMessage);
          // Aksi: Mematikan mesin melalui deep sleep
          enterDeepSleep();
        } else {
          Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Semua parameter dalam batas aman.");
        }
      }
    }
    // Tambahkan delay kecil untuk menghindari penggunaan CPU yang berlebihan
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void setup() {
  // Inisialisasi Serial dengan baud rate yang lebih tinggi
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

  // Inisialisasi Queue
  xSensorQueue = xQueueCreate(10, sizeof(SensorData_t)); // Queue dengan kapasitas 10 item
  if (xSensorQueue == NULL) {
    Serial.println("Gagal membuat queue!");
    while (1); // Hentikan eksekusi jika queue gagal dibuat
  }

  // Inisialisasi Semaphore
  xSemaphoreDataReady = xSemaphoreCreateBinary();
  if (xSemaphoreDataReady == NULL) {
    Serial.println("Gagal membuat binary semaphore!");
    while (1); // Hentikan eksekusi jika semaphore gagal dibuat
  }

  // Koneksi ke Wi-Fi
  Serial.println("Menghubungkan ke Wi-Fi...");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nTerhubung ke Wi-Fi");

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

  // Membuat task
  xTaskCreate(readSensors, "Sensor Task", 4096, NULL, 1, NULL);
  xTaskCreate(blynkTask, "Blynk Task", 4096, NULL, 1, NULL);
}

// Fungsi untuk menerima pembaruan sleep_time dari Blynk
BLYNK_WRITE(VIRTUAL_SET_SLEEP_TIME) {
  int new_sleep_time = param.asInt();
  if (new_sleep_time > 0) {
    sleep_time = new_sleep_time;
    Serial.println("\n================================\n");
    Serial.println("Sleep time diupdate menjadi: ");
    Serial.println(sleep_time);
    Serial.println("\n================================\n");
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
    Serial.println("\n================================\n");
    Serial.println("Safe distance diupdate menjadi: ");
    Serial.println(safe_distance);
    Serial.println("\n================================\n");
    Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Safe distance diupdate menjadi: " + String(safe_distance) + " cm");
  } else {
    Serial.println("Safe distance tidak valid");
    Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Safe distance tidak valid");
  }
}

// Fungsi untuk menerima pembaruan ambang batas CO2 dari Blynk
BLYNK_WRITE(VIRTUAL_THRESHOLD_CO2) {
  threshold_co2 = param.asFloat();
  Serial.println("\n================================\n");
  Serial.println("Ambang batas CO2 diubah menjadi: ");
  Serial.println(threshold_co2);
  Serial.println("\n================================\n");
  Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Ambang batas CO2 diubah menjadi: " + String(threshold_co2) + " ppm");
}

// Fungsi untuk menerima pembaruan ambang batas CO dari Blynk
BLYNK_WRITE(VIRTUAL_THRESHOLD_CO) {
  threshold_co = param.asFloat();
  Serial.println("\n================================\n");
  Serial.println("Ambang batas CO diubah menjadi: ");
  Serial.println(threshold_co);
  Serial.println("\n================================\n");
  Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Ambang batas CO diubah menjadi: " + String(threshold_co) + " ppm");
}

// Fungsi untuk menerima pembaruan ambang batas CH4 dari Blynk
BLYNK_WRITE(VIRTUAL_THRESHOLD_CH4) {
  threshold_ch4 = param.asFloat();
  Serial.println("\n================================\n");
  Serial.println("Ambang batas CH4 diubah menjadi: ");
  Serial.println(threshold_ch4);
  Serial.println("\n================================\n");
  Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Ambang batas CH4 diubah menjadi: " + String(threshold_ch4) + " ppm");
}

// Fungsi untuk menerima pembaruan ambang batas NH3 dari Blynk
BLYNK_WRITE(VIRTUAL_THRESHOLD_NH3) {
  threshold_nh3 = param.asFloat();
  Serial.println("\n================================\n");
  Serial.println("Ambang batas NH3 diubah menjadi: ");
  Serial.println(threshold_nh3);
  Serial.println("\n================================\n");
  Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Ambang batas NH3 diubah menjadi: " + String(threshold_nh3) + " ppm");
}

void loop() {
  // Tidak perlu menjalankan Blynk.run() di sini karena sudah dijalankan dalam blynkTask
  // Namun, jalankan timer.run() jika digunakan
  timer.run();
}
