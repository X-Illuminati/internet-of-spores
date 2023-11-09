# Sensor Software Architecture
This outline is currently a work-in-progress.

## Block Diagram
![Software Stack](drawio/sensorsw_block_diagram.png)  
The sensor node software can be roughly decomposed into a software stack including applications, middleware, drivers, and SDK (which includes low-level drivers).  
There are 2 main applications: Main, which handles normal business logic of the sensor; and the Connection Manager, which handles connection to the WiFi network and interaction with the Node-RED server.  
A third application is the WiFiManager, which is more properly located in the middleware layer since its behavior and configuration are managed by the Connection Manager.  
The driver layer is composed of high-level drivers that are implemented using the Arduino and ESP SDK functionalities.  
The SDK layer might be termed as a "BSP" layer. It provides Arduino HAL functionalities as well as ESP8266-specific low-level drivers.

## External Dependencies
- Arduino
- ESP SDK
- WiFi Manager
- HP303B driver

## Component Description (overview, dependencies, configuration, public API)
### Main
### Static Configuration
### RTC Mem
### Persistent Storage
### Connectivity
#### WiFi Manager
#### Connection Manager
### Sensors
#### SHT30
#### HP303B
#### PPD42
### E-Paper Display

## Dynamic Behavior
### Modes of Operation
- Static Modes
  - Development Mode
  - Tethered Mode
  - VCC Cal Mode
- Dynamic Modes
  - Configuration Mode (WiFi Manager)
  - Init Mode
  - Sleep Mode
  - Normal Mode
  - Connectivity Mode

### Configuration
- Describe double-press reset
- Describe WiFi Manager operation

### Initialization
- Describe init mode (first time boot)

### Sensor Operation
- Describe normal transitions between normal mode and sleep mode
- Describe reset behavior

### Sensor Connectivity
- Describe upload operation
- Describe result processing
- Describe OTA FW and calibration

## Error Handling
- Server Connectivity
- Sensor Upload
- OTA Failures
- Persistent Storage Failures
- Low Battery
- Low Temperature

