#define BLYNK_TEMPLATE_ID "TMPL6HOsfa_3b"
#define BLYNK_TEMPLATE_NAME "Unity Gas Monitoring"
#define BLYNK_AUTH_TOKEN "MA5yxkC-Afvj25fdhqTJfNKe_9hQEdcJ"

// Library yang dibutuhkan
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <math.h> // Untuk fungsi matematika seperti pow()
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

// Definisi PIN
#define MQ135_PIN 34
#define LED_PIN 35
#define BUZZER_PIN 32
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

// Queue Handles
QueueHandle_t Queue_Gas;
QueueHandle_t Queue_Distance;

// Mutex Handle
SemaphoreHandle_t Mutex_Blynk;

// Forward Declarations
void Task_GasSensor(void *pvParameters);
void Task_DistanceSensor(void *pvParameters);
void Task_DataProcessor(void *pvParameters);
void enterDeepSleep();

// Setup Function
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

  // Membuat Queue
  Queue_Gas = xQueueCreate(10, sizeof(float) * 4); // Menyimpan CO2, CO, CH4, NH3
  if (Queue_Gas == NULL) {
    Serial.println("Failed to create Queue_Gas");
  }

  Queue_Distance = xQueueCreate(10, sizeof(float)); // Menyimpan jarak
  if (Queue_Distance == NULL) {
    Serial.println("Failed to create Queue_Distance");
  }

  // Membuat Mutex
  Mutex_Blynk = xSemaphoreCreateMutex();
  if (Mutex_Blynk == NULL) {
    Serial.println("Failed to create Mutex_Blynk");
  }

  // Membuat Tasks
  xTaskCreatePinnedToCore(
    Task_GasSensor,       // Fungsi Task
    "GasSensorTask",     // Nama Task
    2048,                // Ukuran Stack
    NULL,                // Parameter
    1,                   // Prioritas
    NULL,                // Handle Task
    0);                  // Core 0

  xTaskCreatePinnedToCore(
    Task_DistanceSensor, // Fungsi Task
    "DistanceSensorTask",// Nama Task
    2048,                // Ukuran Stack
    NULL,                // Parameter
    1,                   // Prioritas
    NULL,                // Handle Task
    0);                  // Core 0

  xTaskCreatePinnedToCore(
    Task_DataProcessor,  // Fungsi Task
    "DataProcessorTask", // Nama Task
    4096,                // Ukuran Stack
    NULL,                // Parameter
    2,                   // Prioritas Lebih Tinggi
    NULL,                // Handle Task
    1);                  // Core 1
}

// Loop Function
void loop() {
  Blynk.run();
  // Tidak perlu melakukan apa-apa di loop karena semua tugas dikelola oleh FreeRTOS
}

// Task untuk Membaca Sensor Gas
void Task_GasSensor(void *pvParameters) {
  (void) pvParameters;

  while (1) {
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
    Serial.print("MQ-135 - CO2 PPM: ");
    Serial.println(co2_ppm, 2);
    Serial.print("MQ-135 - CO PPM: ");
    Serial.println(co_ppm, 2);
    Serial.print("MQ-135 - CH4 PPM: ");
    Serial.println(methane_ppm, 2);
    Serial.print("MQ-135 - NH3 PPM: ");
    Serial.println(ammonia_ppm, 2);

    // Mengirim data ke Queue_Gas
    float gasData[4] = {co2_ppm, co_ppm, methane_ppm, ammonia_ppm};
    if (xQueueSend(Queue_Gas, &gasData, (TickType_t)10) != pdPASS) {
      Serial.println("Failed to send gas data to Queue_Gas");
    }

    vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1 detik
  }
}

// Task untuk Membaca Sensor Jarak
void Task_DistanceSensor(void *pvParameters) {
  (void) pvParameters;

  while (1) {
    // Bersihkan trigPin
    digitalWrite(SR04_TRIG_PIN, LOW);
    delayMicroseconds(2);

    // Set trigPin HIGH selama 10 mikrodetik
    digitalWrite(SR04_TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(SR04_TRIG_PIN, LOW);

    // Hitung jarak berdasarkan waktu pantulan
    duration = pulseIn(SR04_ECHO_PIN, HIGH, 30000); // Timeout 30 ms
    if (duration == 0) {
      // Tidak ada pantulan
      distanceCm = -1.0;
    } else {
      distanceCm = duration * SOUND_SPEED / 2;
      distanceInch = distanceCm * CM_TO_INCH;
    }

    // Mengirim data ke Queue_Distance
    if (distanceCm >= 0) {
      if (xQueueSend(Queue_Distance, &distanceCm, (TickType_t)10) != pdPASS) {
        Serial.println("Failed to send distance data to Queue_Distance");
      }
    } else {
      Serial.println("Distance out of range");
    }

    vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1 detik
  }
}

// Task untuk Memproses Data dari Queue
void Task_DataProcessor(void *pvParameters) {
  (void) pvParameters;

  float gasData[4];
  float distance;
  bool gasExceeded = false;
  String warningMessage = "";

  while (1) {
    // Menerima data gas
    if (xQueueReceive(Queue_Gas, &gasData, (TickType_t)100) == pdPASS) {
      float co2_ppm = gasData[0];
      float co_ppm = gasData[1];
      float methane_ppm = gasData[2];
      float ammonia_ppm = gasData[3];

      // Memeriksa ambang batas gas
      if (co2_ppm > threshold_co2) {
        warningMessage += "Peringatan: Kadar CO2 melebihi batas aman!\n";
        gasExceeded = true;
      }
      if (co_ppm > threshold_co) {
        warningMessage += "Peringatan: Kadar CO melebihi batas aman!\n";
        gasExceeded = true;
      }
      if (methane_ppm > threshold_ch4) {
        warningMessage += "Peringatan: Kadar Methane (CH4) melebihi batas aman!\n";
        gasExceeded = true;
      }
      if (ammonia_ppm > threshold_nh3) {
        warningMessage += "Peringatan: Kadar Ammonia (NH3) melebihi batas aman!\n";
        gasExceeded = true;
      }
    }

    // Menerima data jarak
    if (xQueueReceive(Queue_Distance, &distance, (TickType_t)100) == pdPASS) {
      // Memeriksa jarak
      if (distance > safe_distance && distance < (safe_distance + 10)) {
        warningMessage += "Peringatan: Pegawai mendekat ke mesin!\n";
        gasExceeded = true;

        // Mengendalikan LED dan Buzzer
        digitalWrite(LED_PIN, HIGH);
        digitalWrite(BUZZER_PIN, HIGH);
      }
      else if (distance <= safe_distance) {
        warningMessage += "Aksi: Mematikan mesin untuk mencegah kecelakaan.\n";
        gasExceeded = true;

        // Mematikan LED dan Buzzer
        digitalWrite(LED_PIN, LOW);
        digitalWrite(BUZZER_PIN, LOW);

        // Memasuki mode deep sleep
        enterDeepSleep();
      }
      else {
        // Jarak aman, matikan LED dan Buzzer jika menyala
        digitalWrite(LED_PIN, LOW);
        digitalWrite(BUZZER_PIN, LOW);
      }
    }

    // Jika ada peringatan, kirim ke Blynk dan masuki deep sleep
    if (gasExceeded) {
      if (xSemaphoreTake(Mutex_Blynk, (TickType_t)10) == pdTRUE) {
        Serial.println(warningMessage);
        Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, warningMessage);
        xSemaphoreGive(Mutex_Blynk);
      }

      // Reset variabel peringatan
      gasExceeded = false;
      warningMessage = "";

      // Memasuki deep sleep
      enterDeepSleep();
    }

    // Mengirim data ke Blynk
    if (xSemaphoreTake(Mutex_Blynk, (TickType_t)10) == pdTRUE) {
      // Kirim data gas
      Blynk.virtualWrite(VIRTUAL_CO2_PIN, gasData[0]);
      Blynk.virtualWrite(VIRTUAL_CO_PIN, gasData[1]);
      Blynk.virtualWrite(VIRTUAL_CH4_PIN, gasData[2]);
      Blynk.virtualWrite(VIRTUAL_NH3_PIN, gasData[3]);

      // Kirim data jarak
      Blynk.virtualWrite(VIRTUAL_DISTANCE_PIN, distanceCm);

      xSemaphoreGive(Mutex_Blynk);
    }

    // Reset warningMessage setelah dikirim
    warningMessage = "";
  }
}

// Fungsi untuk Memasuki Mode Deep Sleep
void enterDeepSleep() {
  // Jalankan timer countdown
  for (int remainingTime = sleep_time; remainingTime >= 0; remainingTime--) {
    if (xSemaphoreTake(Mutex_Blynk, (TickType_t)10) == pdTRUE) {
      Blynk.virtualWrite(VIRTUAL_SLEEP_COUNTDOWN, remainingTime); // Kirim waktu tersisa ke Virtual Pin V7
      Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Memasuki deep sleep dalam " + String(remainingTime) + " detik...");
      xSemaphoreGive(Mutex_Blynk);
    }

    Serial.print("Waktu tersisa sebelum deep sleep: ");
    Serial.println(remainingTime);

    vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1 detik
  }

  // Masuk ke mode deep sleep setelah timer selesai
  if (xSemaphoreTake(Mutex_Blynk, (TickType_t)10) == pdTRUE) {
    Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Memasuki deep sleep...");
    xSemaphoreGive(Mutex_Blynk);
  }

  Serial.println("Memasuki deep sleep...");
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
    if (xSemaphoreTake(Mutex_Blynk, (TickType_t)10) == pdTRUE) {
      Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Sleep time diupdate menjadi: " + String(sleep_time) + " detik");
      xSemaphoreGive(Mutex_Blynk);
    }
  } else {
    Serial.println("Sleep time tidak valid");
    if (xSemaphoreTake(Mutex_Blynk, (TickType_t)10) == pdTRUE) {
      Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Sleep time tidak valid");
      xSemaphoreGive(Mutex_Blynk);
    }
  }
}

// Fungsi untuk menerima pembaruan safe_distance dari Blynk
BLYNK_WRITE(VIRTUAL_SET_SAFE_DISTANCE) {
  int new_safe_distance = param.asInt();
  if (new_safe_distance > 0) {
    safe_distance = new_safe_distance;
    Serial.print("Safe distance diupdate menjadi: ");
    Serial.println(safe_distance);
    if (xSemaphoreTake(Mutex_Blynk, (TickType_t)10) == pdTRUE) {
      Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Safe distance diupdate menjadi: " + String(safe_distance) + " cm");
      xSemaphoreGive(Mutex_Blynk);
    }
  } else {
    Serial.println("Safe distance tidak valid");
    if (xSemaphoreTake(Mutex_Blynk, (TickType_t)10) == pdTRUE) {
      Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Safe distance tidak valid");
      xSemaphoreGive(Mutex_Blynk);
    }
  }
}

// Fungsi untuk menerima pembaruan ambang batas CO2 dari Blynk
BLYNK_WRITE(VIRTUAL_THRESHOLD_CO2) {
  threshold_co2 = param.asFloat();
  Serial.print("Ambang batas CO2 diubah menjadi: ");
  Serial.println(threshold_co2);
  if (xSemaphoreTake(Mutex_Blynk, (TickType_t)10) == pdTRUE) {
    Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Ambang batas CO2 diubah menjadi: " + String(threshold_co2) + " ppm");
    xSemaphoreGive(Mutex_Blynk);
  }
}

// Fungsi untuk menerima pembaruan ambang batas CO dari Blynk
BLYNK_WRITE(VIRTUAL_THRESHOLD_CO) {
  threshold_co = param.asFloat();
  Serial.print("Ambang batas CO diubah menjadi: ");
  Serial.println(threshold_co);
  if (xSemaphoreTake(Mutex_Blynk, (TickType_t)10) == pdTRUE) {
    Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Ambang batas CO diubah menjadi: " + String(threshold_co) + " ppm");
    xSemaphoreGive(Mutex_Blynk);
  }
}

// Fungsi untuk menerima pembaruan ambang batas CH4 dari Blynk
BLYNK_WRITE(VIRTUAL_THRESHOLD_CH4) {
  threshold_ch4 = param.asFloat();
  Serial.print("Ambang batas CH4 diubah menjadi: ");
  Serial.println(threshold_ch4);
  if (xSemaphoreTake(Mutex_Blynk, (TickType_t)10) == pdTRUE) {
    Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Ambang batas CH4 diubah menjadi: " + String(threshold_ch4) + " ppm");
    xSemaphoreGive(Mutex_Blynk);
  }
}

// Fungsi untuk menerima pembaruan ambang batas NH3 dari Blynk
BLYNK_WRITE(VIRTUAL_THRESHOLD_NH3) {
  threshold_nh3 = param.asFloat();
  Serial.print("Ambang batas NH3 diubah menjadi: ");
  Serial.println(threshold_nh3);
  if (xSemaphoreTake(Mutex_Blynk, (TickType_t)10) == pdTRUE) {
    Blynk.virtualWrite(VIRTUAL_MESSAGE_PIN, "Ambang batas NH3 diubah menjadi: " + String(threshold_nh3) + " ppm");
    xSemaphoreGive(Mutex_Blynk);
  }
}