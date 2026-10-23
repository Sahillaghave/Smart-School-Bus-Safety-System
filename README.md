# 🚌 Smart School Bus Safety System

A smart, embedded-system-based solution designed to improve school bus safety by integrating RFID-based identification, GPS location tracking, GSM communication, and an audible buzzer using an ESP32 microcontroller.

## 📌 Project Overview

The **Smart School Bus Safety System** aims to improve student transportation safety through identification and location-tracking features. The ESP32 acts as the central controller, processing RFID inputs, receiving GPS coordinates, and communicating through a GSM module.

The system demonstrates how embedded systems and wireless communication can be used to support safer school transportation.

## 🎯 Objectives

- Identify students using RFID cards.
- Track the bus location using GPS.
- Enable SMS communication through a GSM module.
- Provide audible alerts using a buzzer.
- Integrate multiple hardware modules with an ESP32 microcontroller.

## 🛠️ Hardware Components

| Component | Purpose |
|---|---|
| ESP32 Development Board | Main controller |
| RFID Reader (RC522) | Student identification |
| RFID Cards/Tags | Student identification credentials |
| GPS Module (NEO-6M) | Obtaining location coordinates |
| GSM Module (SIM900A) | SMS communication |
| Buzzer | Audible alerts |
| Power Supply | Powers the electronic components |
| Jumper Wires | Hardware connections |

## 💻 Software and Technologies

- **Programming Language:** C/C++ (Arduino)
- **IDE:** Arduino IDE
- **Microcontroller:** ESP32
- **Communication:** RFID, UART/Serial, GPS, GSM
- **Libraries:** Appropriate ESP32, MFRC522, and GPS libraries depending on the implementation

## ⚙️ Working Principle

1. **Student Identification:** The RFID reader scans a student's RFID card or tag.
2. **Data Processing:** The ESP32 reads the RFID data and processes it according to the programmed logic.
3. **Location Tracking:** The GPS module receives satellite signals and provides geographical coordinates when a valid fix is available.
4. **GSM Communication:** The GSM module can send SMS messages using AT commands when supported by the implemented program and a working cellular connection.
5. **Audible Alert:** The buzzer provides an audible indication according to the programmed conditions.

The exact sequence of operations depends on the firmware uploaded to the ESP32.

## 🧩 System Architecture

```text
       RFID Card / Tag
              |
              v
        RFID Reader
              |
              v
       +--------------+
       |     ESP32    |
       | Main Control |
       +--------------+
         |     |    |
         v     v    v
       GPS    GSM  Buzzer
      Module Module  |
         |     |     v
         v     v   Audible
      Location SMS   Alert
      Coordinates
```

## 🚀 Getting Started

### Prerequisites

- Arduino IDE installed on your computer.
- ESP32 board support configured in Arduino IDE.
- Required component libraries installed.
- ESP32, RFID reader, GPS module, GSM module, buzzer, and appropriate power supplies.

### Installation

1. Clone or download this repository.
2. Open the Arduino source code in Arduino IDE.
3. Install the libraries required by the code.
4. Connect the components according to the circuit diagram and pin configuration.
5. Select the correct ESP32 board and COM port.
6. Upload the program to the ESP32.
7. Open Serial Monitor to inspect output and debug communication.

## 🔌 Important Hardware Notes

- Verify the operating-voltage requirements of every module before connecting it.
- The RC522 RFID reader typically uses 3.3V logic; follow the module's specifications.
- The SIM900A requires a suitable power supply capable of handling its current peaks. Do not assume the ESP32's 3.3V pin can power it.
- Ensure that the ESP32 and connected modules share a common ground where required.
- Verify UART pin assignments and logic-level compatibility before connecting serial devices.
- GSM operation depends on network availability, SIM activation, compatible cellular service, and required SMS service.

## 🌟 Key Features

- RFID-based student identification
- GPS-based location acquisition
- GSM-based SMS communication capability
- Buzzer-based audible indication
- ESP32-based embedded control
- Modular hardware design

## 🔮 Future Improvements

- Parent notification integration
- A real-time bus location dashboard
- Student boarding and departure logs
- A web or mobile application for authorized users
- Improved monitoring and reporting features

*These are potential future enhancements and are not claimed as implemented features.*

## 🎓 Applications

- School transportation safety projects
- Embedded systems and IoT learning
- RFID-based identification systems
- GPS and GSM communication demonstrations
- Academic mini-projects and engineering demonstrations

## 👥 Project Information

**Project Name:** Smart School Bus Safety System  
**Domain:** Embedded Systems / IoT  
**Controller:** ESP32  
**Purpose:** Student identification and school bus safety support#

## 👥 Contributors

- Sahil Laghave — Project Lead
- Rohit Somase  — Hardware Integration
- Shraddha Kandalkar — Programming
- Alfiya Shaikh — Testing and Documentation
