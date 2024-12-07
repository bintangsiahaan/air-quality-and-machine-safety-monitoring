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

void sensorTask(){
  static unsigned long lastTime = 0;
  unsigned long currentTime = millis();

  // Run every second
  if (currentTime - lastTime >= 1000) {
    lastTime = currentTime;
  // Clear the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Set the trigPin HIGH for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Read echoPin and calculate distance
  duration = pulseIn(echoPin, HIGH);
  distanceCm = duration * SOUND_SPEED / 2;
  distanceInch = distanceCm * CM_TO_INCH;
  Blynk.virtualWrite(V1, "Please mind your distance.");
  // Check distance and handle LED + deep sleep
  if (distanceCm > 20 && distanceCm < 30){
    Serial.println("Please step away of the machine");
    Blynk.virtualWrite(V1, "Please step away of the machine");
  }
  if (distanceCm < 20) {
    digitalWrite(BUILTIN_LED, LOW); // Turn off LED
    Serial.println("Object detected within 20 cm. Entering deep sleep...");
    Blynk.virtualWrite(V1, "Object detected within 20 cm. Entering deep sleep...");

    delay(1000); // Small delay for stability
    // Configure deep sleep for 30 seconds
    esp_sleep_enable_timer_wakeup(30 * 1000000); // Time in microseconds
    esp_deep_sleep_start(); // Enter deep sleep
  } else {
    digitalWrite(BUILTIN_LED, HIGH); // Turn on LED
    Serial.print("Distance (cm): ");
    Serial.println(distanceCm);
    Serial.print("Distance (inch): ");
    Serial.println(distanceInch);
  }
  
  // Send distance to Blynk
  Blynk.virtualWrite(V0, distanceCm);

  }
}

void loop() {
  Blynk.run();
  sensorTask();
}
