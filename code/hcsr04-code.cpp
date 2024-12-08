#define BLYNK_TEMPLATE_ID "TMPL6i-ulUeg_"
#define BLYNK_TEMPLATE_NAME "Proyek IOT"
#define BLYNK_AUTH_TOKEN "DMwQtTU-5CFN5GhzERxNW-Kl-NdFFBmw"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

const int trigPin = 26;
const int echoPin = 27;

// Define sound speed in cm/uS
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701

// Wi-Fi credentials
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Hotspot";  // Replace with your Wi-Fi SSID
char pass[] = "n0t33421";  // Replace with your Wi-Fi Password

long duration;
float distanceCm;
float distanceInch;
int sleep_time = 30; // Default sleep time in seconds
int safe_distance = 20;

void setup() {
  Serial.begin(9600); // Start serial communication
  pinMode(trigPin, OUTPUT); // Set trigPin as an Output
  pinMode(echoPin, INPUT); // Set echoPin as an Input
  pinMode(BUILTIN_LED, OUTPUT); // Set LED pin as Output
  
  // Connect to Wi-Fi
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");

  // Test DNS Resolution for Blynk
  IPAddress ip;
  if (WiFi.hostByName("blynk-cloud.com", ip)) {
    Serial.println("Resolved Blynk IP: " + ip.toString());
  } else {
    Serial.println("DNS resolution failed");
  }

  // Connect to Blynk
  Serial.println("Connecting to Blynk...");
  Blynk.begin(auth, ssid, pass);

  if (Blynk.connected()) {
    Serial.println("Connected to Blynk");
  } else {
    Serial.println("Failed to connect to Blynk");
  }

  // Print wakeup reason after deep sleep
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
    Serial.println("Woke up from deep sleep");
    Blynk.virtualWrite(V1, "Woke up from deep sleep");
  } else {
    Serial.println("Power-on or reset");
  }
}

void enterDeepSleep() {
  // Jalankan timer countdown
  for (int remainingTime = sleep_time; remainingTime >= 0; remainingTime--) {
    Blynk.virtualWrite(V2, remainingTime); // Kirim waktu tersisa ke Virtual Pin V2
    Serial.print("Time remaining before deep sleep: ");
    Serial.println(remainingTime);

    delay(1000); // Tunggu 1 detik sebelum memperbarui waktu
  }

  // Masuk ke mode deep sleep setelah timer selesai
  Serial.println("Entering deep sleep...");
  Blynk.virtualWrite(V1, "Entering deep sleep...");
  esp_sleep_enable_timer_wakeup(sleep_time * 1000000); // Set deep sleep timer
  esp_deep_sleep_start(); // Masuk ke deep sleep
}

void sensorTask() {
  static unsigned long lastTime = 0;
  unsigned long currentTime = millis();

  // Jalankan setiap 1 detik
  if (currentTime - lastTime >= 1000) {
    lastTime = currentTime;

    // Bersihkan trigPin
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    // Set trigPin HIGH selama 10 mikrodetik
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    // Hitung jarak berdasarkan waktu pantulan
    duration = pulseIn(echoPin, HIGH);
    distanceCm = duration * SOUND_SPEED / 2;
    distanceInch = distanceCm * CM_TO_INCH;

    // Kirim data jarak ke Blynk (Virtual Pin V0)
    Blynk.virtualWrite(V0, distanceCm);
    Blynk.virtualWrite(V1, "Please mind your distance.");
    // Check distance and handle LED + deep sleep
    if (distanceCm > safe_distance && distanceCm < (safe_distance + 10)){
      Serial.println("Please step away of the machine");
      Blynk.virtualWrite(V1, "Please step away of the machine");
    }
    // Logika berdasarkan jarak
    if (distanceCm < safe_distance) {
      digitalWrite(BUILTIN_LED, LOW); // Matikan LED
      Serial.println("Object detected within " + String(safe_distance) +  " cm.");
      Blynk.virtualWrite(V1, "Object detected within " + String(safe_distance) +  " cm. Preparing for deep sleep...");

      delay(100); // Stabilitas data
      enterDeepSleep(); // Masuk ke mode deep sleep
    } else {
      digitalWrite(BUILTIN_LED, HIGH); // Nyalakan LED
      Serial.print("Distance (cm): ");
      Serial.println(distanceCm);
      Serial.print("Distance (inch): ");
      Serial.println(distanceInch);
    }
  }
}

BLYNK_WRITE(V3) {
  // Update sleep_time when a new value is received on Virtual Pin V3
  int new_sleep_time = param.asInt();
  if (new_sleep_time > 0) {
    sleep_time = new_sleep_time;
    Serial.print("Updated sleep_time to: ");
    Serial.println(sleep_time);
    Blynk.virtualWrite(V1, "Sleep time updated to: " + String(sleep_time) + " seconds");
    delay(2000);
  } else {
    Serial.println("Invalid sleep time received");
    Blynk.virtualWrite(V1, "Invalid sleep time received");
  }
}

BLYNK_WRITE(V4) {
  // Update sleep_time when a new value is received on Virtual Pin V3
  int new_safe_distance = param.asInt();
  if (new_safe_distance > 0) {
    safe_distance = new_safe_distance;
    Serial.print("Updated safe_distance to: ");
    Serial.println(safe_distance);
    Blynk.virtualWrite(V1, "Safe distance updated to: " + String(safe_distance) + " cm");
    delay(2000);
  } else {
    Serial.println("Invalid safe distance received");
    Blynk.virtualWrite(V1, "Invalid safe distance received");
  }
}

void loop() {
  Blynk.run();
  sensorTask();
}