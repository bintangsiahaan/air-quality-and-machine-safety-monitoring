#define BLYNK_TEMPLATE_ID "TMPL6FBYn5Tsq"
#define BLYNK_TEMPLATE_NAME "gas monitoring"
#define BLYNK_AUTH_TOKEN "T0Nj9LUpSEeP4hsXvzT_GuVCm59c5ZhP"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>  

#define MQ135_PIN 34

// Koneksi WiFi
const char* ssid = "HNIS";  
const char* password = "qwerty90";  

// Virtual Pins untuk menampilkan nilai gas
#define VIRTUAL_CO2_PIN V1
#define VIRTUAL_CO_PIN V5
#define VIRTUAL_CH4_PIN V6
#define VIRTUAL_NH3_PIN V7

// Virtual Pins untuk mengatur ambang batas
#define VIRTUAL_THRESHOLD_CO2 V10
#define VIRTUAL_THRESHOLD_CO V11
#define VIRTUAL_THRESHOLD_CH4 V12
#define VIRTUAL_THRESHOLD_NH3 V13

// Ambang batas default (ppm)
float threshold_co2 = 1000.0;   // Contoh: 1000 ppm CO2
float threshold_co = 50.0;       // Contoh: 50 ppm CO
float threshold_ch4 = 100.0;     // Contoh: 100 ppm CH4
float threshold_nh3 = 25.0;      // Contoh: 25 ppm NH3

BlynkTimer timer;

void setup() {
  Serial.begin(115200);
  pinMode(MQ135_PIN, INPUT);

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("...");
  }
  Serial.println("\nWiFi Connected!"); 

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);  

  // Set interval untuk membaca sensor setiap 1 detik
  timer.setInterval(1000L, readSensor);
}

void loop() {
  Blynk.run();
  timer.run();
}

// Fungsi untuk membaca sensor dan mengirim data
void readSensor() {
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
  Serial.print("Raw Analog Value: ");
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

  // Mengecek apakah nilai gas melebihi ambang batas dan mengirim peringatan melalui Serial
  if (co2_ppm > threshold_co2) {
    Serial.println("Peringatan: Kadar CO2 melebihi batas aman!");
  }
  if (co_ppm > threshold_co) {
    Serial.println("Peringatan: Kadar CO melebihi batas aman!");
  }
  if (methane_ppm > threshold_ch4) {
    Serial.println("Peringatan: Kadar Methane (CH4) melebihi batas aman!");
  }
  if (ammonia_ppm > threshold_nh3) {
    Serial.println("Peringatan: Kadar Ammonia (NH3) melebihi batas aman!");
  }
}

// Fungsi untuk menerima pembaruan ambang batas dari Blynk
BLYNK_WRITE(VIRTUAL_THRESHOLD_CO2) {
  threshold_co2 = param.asFloat();
  Serial.print("Ambang batas CO2 telah diubah menjadi: ");
  Serial.println(threshold_co2);
}

BLYNK_WRITE(VIRTUAL_THRESHOLD_CO) {
  threshold_co = param.asFloat();
  Serial.print("Ambang batas CO telah diubah menjadi: ");
  Serial.println(threshold_co);
}

BLYNK_WRITE(VIRTUAL_THRESHOLD_CH4) {
  threshold_ch4 = param.asFloat();
  Serial.print("Ambang batas CH4 telah diubah menjadi: ");
  Serial.println(threshold_ch4);
}

BLYNK_WRITE(VIRTUAL_THRESHOLD_NH3) {
  threshold_nh3 = param.asFloat();
  Serial.print("Ambang batas NH3 telah diubah menjadi: ");
  Serial.println(threshold_nh3);
}
