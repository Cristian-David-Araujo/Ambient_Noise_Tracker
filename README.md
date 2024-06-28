Ambient Noise Tracker
=====================

Overview
--------

The **Ambient Noise Tracker** is a portable device designed to measure and geolocate environmental noise events. The project highlights both hardware and software design to ensure portability and energy efficiency. The device has been tested across various areas of the Universidad de Antioquia, demonstrating accurate noise measurements and low power consumption.

Features
--------

*   **Real-time Noise Monitoring:** Measures environmental noise levels in real-time.
    
*   **Geolocation:** Integrates a GPS module to tag noise data with location information.
    
*   **Energy Efficiency:** Implements low power consumption modes to extend battery life.

The image below allows you to see which modules are part of the system:
![Block Diagram](Block_Diagram.jpg)

Hardware Components
-------------------

*   **Microphone Module:** Captures ambient noise.
    
*   **GPS Module (NEO-6MV2):** Provides location data.
    
*   **Raspberry Pi Pico:** Serves as the microcontroller.
    
*   **LCD Display:** Visualizes data.
    
*   **Lithium Battery (4.2V):** Powers the device.
    
*   **Transistors:** Used as switches to manage power to the modules.

The simplified schematics: 
![alt text](Schematics.jpg)

Software Design
---------------

*   **Data Acquisition:** Collects noise data and GPS coordinates.
    
*   **Signal Processing:** Processes the collected data to calculate noise levels.
    
*   **Visualization:** Displays results on the LCD and logs data for further analysis.
    

Power Consumption
-----------------

*   **DORMANT Mode:** 0.062 Amperes (minimal power consumption).
    
*   **SLEEP Mode (wfi):** 0.155 Amperes (low power waiting for interruptions).
    
*   **Active Mode:** 0.162 Amperes (during data transfers and processing).
    

Results
-------

*   The device successfully recorded noise levels across the Universidad de Antioquia.
    
*   The highest noise level was recorded at the Río entrance (77.15 dB).
    
*   The lowest noise level was at the Barranquilla entrance (71.00 dB).

The next image sumarize the data adquisition in each location:

![Data Visualization](Noise_Data_dB.png)

Installation
------------

1.  Clone the repository:

```bash
git clone https://github.com/Cristian-David-Araujo/Ambient_Noise_Tracker.git
```

    
2.  Navigate to the project directory:
```bash
cd Ambient_Noise_Tracker
```
    
3.  Install dependencies (if applicable).
    

Usage
-----

To start the noise tracker, power on the device by pressing the button and wait for the GPS to connect. This will be indicated by a green LED. Then, press the button again to start measuring for 10 seconds. Once that time has passed, the system will automatically turn off, allowing you to measure again.

The LCD will display the current location. The data will be stored in non-volatile memory (Flash memory). It is printed on the console every time the system is powered on, so you can copy and paste the data to visualize it.

License
-----
This project is licensed under the MIT License. See the LICENSE file for details.
