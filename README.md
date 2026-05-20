# IoT Smart Classroom Occupancy & Availability Monitoring System

![License](https://img.shields.io/badge/License-MIT-blue.svg)
![Platform](https://img.shields.io/badge/Platform-ESP32-orange.svg)
![Framework](https://img.shields.io/badge/Framework-Arduino-blue)

A real-time, IoT-based smart classroom monitoring system designed to track room availability and occupancy. This project utilizes an ESP32 microcontroller, Time-of-Flight (ToF) laser sensors, and Passive Infrared (PIR) sensors to autonomously determine if a classroom is empty, occupied only by a lecturer, occupied only by students, or fully in use. 

Data is pushed asynchronously to the **Blynk IoT Platform** for real-time visualization and to **Google Sheets** (via Google Apps Script) for historical data logging.

## Key Features

* **Real-Time Occupancy Tracking:** Uses dual VL53L0X ToF sensors with a custom 4-step State Machine to accurately count people entering and exiting while preventing double-counting.
* **Sticky Latching Presence Detection:** Uses PIR sensors to detect human body heat. The "Lecturer" sensor uses a latching logic to keep the room status active even if the lecturer stands still for a moment.
* **Asynchronous HTTP Queue:** Prevents sensor "freeze" or "lag" during HTTP POST requests. Events are pushed to a Circular Buffer Array and processed in the background, ignoring HTTP 302 redirects to prevent `400 Bad Request` errors.
* **Hardware I2C Bypass:** Solves the identical default I2C address issue of the VL53L0X modules by using the `XSHUT` pins to re-register their addresses to `0x30` and `0x31` dynamically during boot.
* **NTP Time-Blocking (Academic Scheduling):** The system synchronizes with `pool.ntp.org` and compares the current time against a hardcoded academic schedule array. It automatically enters an "Out of Schedule" sleep state to prevent spam notifications and save database rows during off-hours.

---

## Hardware Requirements (BOM)

| Component | Quantity | Description / Notes |
| :--- | :---: | :--- |
| **ESP32 Development Board** | 1 | 30-pin or 38-pin NodeMCU variant. |
| **VL53L0X (GY-530)** | 2 | Time-of-Flight distance sensors for door counting. |
| **HC-SR501 PIR Sensor** | 2 | Motion sensors. **Must be powered via 5V (VIN)** to prevent internal regulator brownouts. |
| **Jumper Wires** | Varies | Male-to-Male and Male-to-Female. |
| **Power Supply** | 1 | 5V/2A Micro-USB or Type-C adapter. |

---

## Hardware Architecture & Wiring

![Hardware Architecture Diagram](placeholder_for_hardware_architecture_diagram.png)


### Pinout Configuration

| Component | ESP32 Pin | Signal Type | Notes |
| :--- | :--- | :--- | :--- |
| **PIR Sensor (Lecturer)** | `GPIO 27` | Digital Input | Detects lecturer presence. |
| **PIR Sensor (Student)** | `GPIO 26` | Digital Input | Detects student presence. |
| **VL53L0X (Both) - SDA**| `GPIO 21` | I2C Data | Shared I2C data bus. |
| **VL53L0X (Both) - SCL**| `GPIO 22` | I2C Clock | Shared I2C clock bus. |
| **VL53L0X (Entry) - XSHUT**| `GPIO 14` | Digital Output | Used to power-cycle and change I2C address. |
| **VL53L0X (Exit) - XSHUT** | `GPIO 13` | Digital Output | Used to power-cycle and change I2C address. |
| **PIR VCC (Both)** | `VIN / 5V` | Power | Do not use 3.3V; HC-SR501 requires 5V. |
| **VL53L0X VCC (Both)** | `3V3` | Power | Safe to use 3.3V. |

> **Important Warning:** Do NOT use `GPIO 12` for the `XSHUT` pins. GPIO 12 is a strapping pin on the ESP32. Pulling it HIGH during boot will cause the internal flash memory voltage to drop to 1.8V, resulting in a fatal `Packet content transfer stopped` error during firmware upload.

---

## Software Requirements & Dependencies

1. **Arduino IDE:** Version 2.3.0 or newer recommended.
2. **ESP32 Core:** Install via Boards Manager (Espressif Systems).
3. **Required Libraries:**
   * `Blynk` by Volodymyr Shymanskyy (v1.3.2+)
   * `VL53L0X` by Pololu (v1.3.1+)
   * Built-in libraries: `WiFi.h`, `Wire.h`, `HTTPClient.h`, `time.h`

---

## Cloud Configuration

### 1. Blynk IoT Setup
Create a new template in your Blynk Console and set up the following **Datastreams (Virtual Pins)**:
* `V0` (Integer): Lecturer Status (0 or 1)
* `V1` (Integer): Student Status (0 or 1)
* `V2` (Integer): Total Occupancy Count
* `V3` (String): Room Status Text (e.g., "Ruang Digunakan", "Di Luar Jadwal")

Update the credentials at the top of the `.ino` file:
```cpp
#define BLYNK_TEMPLATE_ID "Your_Template_ID"
#define BLYNK_TEMPLATE_NAME "Your_Template_Name"
#define BLYNK_AUTH_TOKEN "Your_Auth_Token"

```

### 2. Google Apps Script Setup (Webhook)

To log data to Google Sheets without a third-party service, create a new Google Sheet, go to `Extensions > Apps Script`, and paste the following code:

```javascript
function doPost(e) {
  var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  var data = JSON.parse(e.postData.contents);
  
  // Wipe old data when ESP32 reboots (optional, keeps data fresh)
  if (data.event === "PERANGKAT_MENYALA") {
    var lastRow = sheet.getLastRow();
    if (lastRow > 1) {
      sheet.getRange(2, 1, lastRow - 1, sheet.getLastColumn()).clearContent();
    }
  }
  
  // Append new event log
  sheet.appendRow([
    new Date(),     // Timestamp
    data.jadwal,    // Academic Schedule Slot
    data.waktu,     // Time (WIB)
    data.event,     // MASUK, KELUAR, or JADWAL_BARU
    data.total,     // Total Occupancy
    data.status     // Room Status String
  ]);
  
  // Return success to the ESP32
  return ContentService.createTextOutput("Success");
}

```

Deploy the script as a **Web App**, set access to **"Anyone"**, and paste the generated URL into the `googleScriptURL` variable in your Arduino code.

---

## Installation & Usage

1. Assemble the hardware according to the wiring table. Mount the ToF sensors on the doorframe at chest level (approx. 120cm) to avoid counting legs twice.
2. Open the `.ino` file in Arduino IDE.
3. Replace the `ssid` and `pass` variables with your WiFi credentials.
4. Replace the Blynk credentials and Google Script URL.
5. Connect your ESP32, select the correct COM port and Board, and click **Upload**.
6. Open the Serial Monitor at `115200` baud rate to view the detailed booting sequence and background queue logs.

---

## System Limitations & Realistic Constraints

Based on rigorous Black-box testing, this system has the following known hardware limitations:

* **Crowd Counting Error Rate (~13.33%):** The VL53L0X ToF sensor acts as a single-point ranging laser. If two individuals walk through the door simultaneously side-by-side (shoulder-to-shoulder), the laser will likely read them as a single solid shadow.
* **PIR Temperature Bias:** The HC-SR501 PIR sensor relies on infrared heat signatures. Sudden extreme changes in environmental temperature (e.g., direct sunlight hitting the sensor or an AC unit blowing directly on it) may trigger false-positive presence detections.
* **Data Transmission Dependency:** The system relies strictly on a 2.4GHz WiFi connection. If the network drops, HTTP requests to Google Sheets will fail, though the ESP32 will attempt to reconnect autonomously using the `BlynkTimer`.

---

## License

This project is open-source and available under the [MIT License](https://www.google.com/search?q=LICENSE).

```

```
