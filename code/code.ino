#define BLYNK_TEMPLATE_ID "TMPL6xKhf91P4"  
#define BLYNK_TEMPLATE_NAME "ProyekIoT"     
#define BLYNK_AUTH_TOKEN "3gIBmicADXwbN64tFhpvvMLIjFXNxncc"  

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>  

#define MQ135_PIN 34


const char* ssid = "bluesky";  
const char* password = "24242424";  


#define VIRTUAL_CO2_PIN V1
#define VIRTUAL_CO_PIN V5
#define VIRTUAL_CH4_PIN V6
#define VIRTUAL_NH3_PIN V7

void setup() {
  Serial.begin(115200);
  pinMode(MQ135_PIN, INPUT);

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("...");
  }
  Serial.println("WiFi Connected!"); 

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);  
}

void loop() {
  // membaca nilai dari sensor MQ-135
  int rawValue = analogRead(MQ135_PIN);  // membaca nilai analog dari sensor
  float voltage = rawValue / 1024.0 * 5.0;  // menghitung tegangan
  float resistance = (5.0 - voltage) / voltage;  // menghitung resistansi sensor

  // menghitung konsentrasi gas (ppm) berdasarkan resistansi
  float co2_ppm = 1.0 / (0.03588 * pow(resistance, 1.336));  
  float co_ppm = co2_ppm / 2.2;        
  float methane_ppm = co2_ppm / 2.7;   
  float ammonia_ppm = co2_ppm / 3.6;   

  // menampilkan hasil pembacaan sensor di Serial Monitor
  Serial.print("Raw Analog Value: ");
  Serial.println(rawValue);
  Serial.print("Voltage: ");
  Serial.println(voltage, 2);    // menampilkan tegangan dengan 2 digit desimal
  Serial.print("Sensor Resistance: ");
  Serial.println(resistance, 2); 
  
  // menampilkan nilai ppm untuk CO, CO₂, NH₃, dan CH₄
  Serial.print("CO2 PPM: ");
  Serial.println(co2_ppm, 2);       
  
  Serial.print("CO PPM: ");
  Serial.println(co_ppm, 2);         
  
  Serial.print("Methane (CH4) PPM: ");
  Serial.println(methane_ppm, 2);    
  
  Serial.print("Ammonia (NH3) PPM: ");
  Serial.println(ammonia_ppm, 2);    

  // mengirim data ke Blynk melalui Virtual Pins
  Blynk.virtualWrite(VIRTUAL_CO2_PIN, co2_ppm);  
  Blynk.virtualWrite(VIRTUAL_CO_PIN, co_ppm);     
  Blynk.virtualWrite(VIRTUAL_CH4_PIN, methane_ppm); 
  Blynk.virtualWrite(VIRTUAL_NH3_PIN, ammonia_ppm);  

  delay(1000);  // menunggu 1 detik sebelum pembacaan berikutnya

  Blynk.run();  // menjaga agar Blynk tetap berjalan
}