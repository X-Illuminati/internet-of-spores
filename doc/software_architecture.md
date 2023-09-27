# Sensor Software Architecture
This outline is currently a work-in-progress.

## Block Diagram
Show component decomposition (don't forget external components)

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
#### ESP Updater
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

