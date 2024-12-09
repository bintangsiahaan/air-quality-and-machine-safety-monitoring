# 🌀 Air Quality and Machine Safety Monitoring System

<p align="center">
  <a href="#contributors">Contributors</a> •
  <a href="#introduction-to-the-problem-and-the-solution">Introduction</a> •
  <a href="#hardware-design-and-implementation">Hardware Design</a> •
  <a href="#network-infrastructure">Network Infrastructure</a> •
  <a href="#software-implementation-details">Software Implementation</a> •
  <a href="#test-results-and-performance-evaluation">Test Results</a> •
  <a href="#conclusion-and-future-work">Conclusion</a>
</p>

---

## 📜 Contributors
This project is the Real-Time System & IoT practicum final assignment created by the following group members:

- [Bintang Siahaan](https://github.com/bintangsiahaan) - 2206024322  
- [Hanif Nur Ilham Sanjaya](https://github.com/Ashennwitch) - 2206059692  
- [Muhammad Nadhif Fasichul Ilmi](https://github.com/Nadhiefilmi) - 2206813416  
- [Zikri Zulfa Azhim](https://github.com/verszz) - 2206028390  

This project is part of the undergraduate program in Computer Engineering, Department of Electrical Engineering, Faculty of Engineering, Universitas Indonesia.

---

## 🌍 Introduction to the problem and the solution
Industrial automation has revolutionized the way factories and manufacturing facilities operate. However, alongside increased efficiency and productivity, new challenges have emerged, particularly in ensuring worker safety and maintaining the optimal performance of machinery. Industrial environments often expose workers to hazardous conditions, such as exposure to harmful gases or unsafe proximity to machinery. These risks necessitate robust solutions that continuously monitor environmental conditions, identify potential hazards, and issue timely alerts to enhance workplace safety.

The Air Quality and Machine Safety Monitoring System is an advanced IoT-based solution specifically designed to address these challenges. It provides real-time monitoring of air quality and ensures worker safety by keeping track of their proximity to hazardous machinery. This system plays a critical role in industrial safety management by integrating cutting-edge technology with an easy-to-use monitoring interface.

At the heart of the system lies the ESP32 microcontroller, which acts as the central processing unit. The system utilizes the MQ-135 gas sensor to detect various harmful gases, including carbon monoxide (CO), carbon dioxide (CO₂), ammonia (NH₃), and methane (CH₄). Additionally, the HC-SR04 ultrasonic sensor measures the distance between workers and machinery, ensuring a safe working environment by identifying hazardous proximity.

Collected data is sent to the Blynk application, which serves as a real-time visualization and control platform. This allows factory managers and safety officers to remotely monitor air quality, view proximity data, and receive instant alerts for unsafe conditions. The integration of these technologies not only improves workplace safety but also enhances decision-making by providing actionable insights into the factory’s operational environment.

---

## 🔧 Hardware Design and Implementation
The system integrates the following components:

### 1. **ESP32 Microcontroller**
- Serves as the main processing unit.
- Handles Wi-Fi communication for data transmission to the Blynk platform.

### 2. **MQ-135 Gas Sensor**
- Detects harmful gases, including CO, CO₂, NH₃, and CH₄.
- Continuously monitors air quality by relaying data to the ESP32.

### 3. **HC-SR04 Ultrasonic Sensor**
- Measures the distance between workers and machinery.
- Ensures safety by identifying unsafe proximity.

### 4. **Output Devices**
- **LEDs and Buzzers**: Provide visual and audible alerts for unsafe conditions.  

The ESP32 collects data from the sensors and transmits it to the Blynk platform via Wi-Fi. If any unsafe conditions are detected (e.g., poor air quality or proximity hazards), the system triggers alerts through LEDs and buzzers.

---

## 🌐 Network Infrastructure
The system uses the following communication setup:
- **Wi-Fi Connection**: The ESP32 connects to the internet and sends data to the Blynk platform.
- **Blynk Application**: Provides real-time data visualization and control.  

### Key Functionalities via Blynk:
- View air quality and proximity data.
- Receive real-time alerts for unsafe conditions.
- Remotely adjust safety parameters.  

Wi-Fi credentials (SSID, password) and the Blynk authentication token are securely configured in the source code.

---

## 💻 Software Implementation Details
The system is implemented using **FreeRTOS** to manage multiple tasks concurrently. Key tasks include:

### 1. **Gas Sensor Reading Task**
- Reads data from the MQ-135 sensor.
- Checks gas concentration against predefined safety thresholds.

### 2. **Ultrasonic Sensor Reading Task**
- Monitors worker proximity to machinery.
- Triggers alerts when the distance falls below a safe range.

### 3. **Wi-Fi and Blynk Task**
- Maintains a stable Wi-Fi connection.
- Sends sensor data to the Blynk platform for remote monitoring.

### 4. **Alert Task**
- Activates LEDs and buzzers for visual and audible warnings.  

To avoid task conflicts, mutexes are used for synchronized access to shared resources.

---

## 📊 Test Results and Performance Evaluation
The system has been thoroughly tested in a simulated factory environment.  

### Key Observations:
1. Successfully detects harmful gases such as CO, CO₂, NH₃, and CH₄ using the MQ-135 sensor.
2. Measures safe working distances with the HC-SR04 sensor.
3. Provides early warnings through buzzers and visualizes real-time data on the Blynk app.  

Some inconsistencies in sensor readings were observed, primarily due to hardware quality limitations. However, overall system performance is reliable and effective.

---

## 🔮 Conclusion and Future Work
The **Air Quality and Machine Safety Monitoring System** offers a practical and cost-effective solution to enhance industrial safety. By leveraging the ESP32 microcontroller, MQ-135 gas sensor, and HC-SR04 ultrasonic sensor, the system monitors air quality and provides safety alerts in real time.  

### Future Enhancements:
1. Incorporate more precise sensors for higher accuracy.
2. Add support for monitoring multiple machines simultaneously.
3. Implement machine learning to predict equipment failures and enable predictive maintenance.  

This system demonstrates the potential of IoT in transforming industrial safety standards.

---
