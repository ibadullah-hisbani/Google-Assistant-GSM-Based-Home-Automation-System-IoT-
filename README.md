# Google Assistant & GSM Based Home Automation System (IoT)
Built this for my final year project: a home automation setup that responds to Google Assistant through an ESP32, but doesn't fully depend on internet — added a GSM module (SIM800L) so appliances can still be controlled via SMS if the WiFi's down. Telecom Engineering FYP, QUEST.


## 📌 Project Overview
This project is a **dual-mode smart home automation system** designed to control home appliances through two independent methods — an **online voice/app-based control path** and an **offline GSM/SMS-based fallback path** — ensuring uninterrupted appliance control even during internet outages.

## ⚙️ How It Works
- **Online Mode:** An **ESP32** microcontroller connects to the **SinricPro** cloud platform, enabling voice control through **Google Assistant** and remote app-based control from anywhere with an internet connection.
- **Offline Mode:** When internet is unavailable, a **SIM800L GSM module** paired with an **Arduino Uno** allows appliances to be controlled via simple **SMS commands**, ensuring the system remains functional in areas with poor or no internet connectivity.
- A **4-channel relay module** switches the connected appliances (fan, plug socket, and two bulbs) based on commands received from either control path.
- **Manual override push buttons** are also included, allowing physical control of appliances independent of both the app and SMS systems.

## 🔧 Hardware Components
- ESP32 Microcontroller
- Arduino Uno
- SIM800L GSM Module
- 4-Channel Relay Module
- LM2596 Buck Converter (9V → 4V, for stable SIM800L power supply)
- SMPS Power Supply
- Push Buttons (manual override)
- Voltage Divider Circuit (for ESP32 RX pin protection)

## 💻 Software & Platforms
- Arduino IDE (Embedded C)
- SinricPro Cloud Platform
- Google Home / Google Assistant Integration
- AT Commands (GSM communication)

## 🎯 Key Features
- Dual-mode control: Online (voice/app) + Offline (SMS)
- Reliable fallback mechanism during internet downtime
- Physical manual override for emergency control
- Stable power management for GSM module using buck converter
- Low-cost, scalable smart home solution

## 👨‍💻 Team
- **Ibadullah Hisbani** (22TC-45) — Group Leader
- Muhammad Jaffar (22TC-29)
- Aijaz Ahmed Ghoto (22TC-22)

**Supervisor:** Engr. Mujeed-ur-Rehman

## 🎓 Academic Info
Final Year Project — BE Telecommunication Engineering
Quaid-e-Awam University of Engineering, Sciences & Technology (QUEST), Nawabshah
