# Sensor Software Architecture
This outline is currently a work-in-progress.

## Table of Contents
* [Block Diagram](#block-diagram)
* [External Dependencies](#external-dependencies)
* [Component Description](#component-description)
  - [Main](#main)
  - [Project Configuration](#project-configuration)
  - [Connectivity](#connectivity)
    + [Connection Manager](#connection-manager)
    + [Update Parser](#update-parser)
    + [WiFi Manager](#wifi-manager)
  - [Sensors](#sensors)
    + [SHT30](#sht30)
    + [HP303B](#hp303b)
    + [Pulse2](#pulse2)
  - [RTC Mem](#rtc-mem)
  - [Persistent Storage](#persistent-storage)
  - [E-Paper Display](#e-paper-display)
* [Dynamic Behavior](#dynamic-behavior)

---

## Block Diagram
![Software Stack](drawio/sensorsw_block_diagram.png)  
The sensor node software can be roughly decomposed into a software stack including applications, middleware, drivers, and SDK (which includes low-level drivers).  
There are 2 main applications: Main, which handles normal business logic of the sensor; and the Connection Manager, which handles connection to the WiFi network and interaction with the Node-RED server.  
A third application is the WiFiManager, which is more properly located in the middleware layer since its behavior and configuration are managed by the Connection Manager.  
The driver layer is composed of high-level drivers that are implemented using the Arduino and ESP SDK functionalities.  
The SDK layer provides Arduino HAL functionalities as well as ESP8266-specific low-level drivers.

---

## External Dependencies
| External Dependency | Tested Version | Link
|---------------------|----------------|------
| Arduino IDE         | 2.0.3          | https://www.arduino.cc/
| ESP SDK             | 3.0.2          | https://github.com/esp8266/Arduino
| WiFi Manager        | 0.16.0         | https://github.com/tzapu/WiFiManager
| Lolin HP303B        | commit 4deb5f  | https://github.com/wemos/LOLIN_HP303B_Library

---

## Component Description
 (description, dependencies, configuration, public API)
### Main

### Project Configuration

### Connectivity
#### Connection Manager

#### Update Parser

#### WiFi Manager

### Sensors
##### Description
![Sensors Component Overview](drawio/sensorsw_sensors_overview.png)  
The Sensors component manages the different sensors attached to the sensor node and stores their readings in RTC memory. It can store the following types of sensor readings:
* SENSOR_TEMPERATURE
* SENSOR_HUMIDITY
* SENSOR_PRESSURE
* SENSOR_PARTICLE_1_0
* SENSOR_PARTICLE_2_5
* SENSOR_BATTERY_VOLTAGE

Additionally, the pseudo-sensor type SENSOR_TIMESTAMP_OFFS, is used to store a correlated timestamp with each batch of readings.

##### Dependencies
| Component             | Interface Type     | Description
|-----------------------|--------------------|-------------
| Pulse2                | class              | GPIO pulse duration measurement for PPD42 LPO sensor
| SHT30                 | function           | I2C driver for SHT30 sensor
| HP303B                | class              | I2C driver for HP303B sensor
| RTC Mem               | global, function   | Storage for sensor readings, uptime calculation
| Wiring                | function           | GPIO HAL
| TwoWire               | class              | I2C initialization
| ADC_VCC               | function           | VCC (battery) Voltage sensor HAL
| Project Configuration | preprocessor macro | Configuration settings
| Serial                | class              | Logging printf

##### Configuration
Configuration of this component is done through preprocessor defines set in project_config.h.

| Configuration | Type    | Description
|---------------|---------|-------------
| EXTRA_DEBUG   | bool    | Enables additional debug logging
| TETHERED_MODE | bool    | Enables PPD42 sensor
| SHT30_ADDR    | uint8_t | I2C Address for the SHT30 sensor
| PPD42_PIN_DET | uint8_t | Pin # used to detect presence of PPD42 sensor
| PPD42_PIN_1_0 | uint8_t | Pin # used as LPO output of PPD42 sensor for PM1.0 detections
| PPD42_PIN_2_5 | uint8_t | Pin # used as LPO output of PPD42 sensor for PM2.5 detections

##### Public API

###### Types and Enums
sensor_type_t
> This enum provides labels for the different types of sensor readings.
>
> Enumerations:
> * SENSOR_UNKNOWN
> * SENSOR_TEMPERATURE
> * SENSOR_HUMIDITY
> * SENSOR_PRESSURE
> * SENSOR_PARTICLE_1_0
> * SENSOR_PARTICLE_2_5
> * SENSOR_BATTERY_VOLTAGE
> * SENSOR_TIMESTAMP_OFFS

###### Functions
sensors_init
> Initialize module. 
>
> | Parameter     | Direction | Type          | description
> |---------------|-----------|---------------|-------------
> |               | return    | void          |

read_ppd42
> Read and store values from the PPD42 particle sensor.
> This measurement is recommended to take 30 seconds.
>
> 🪧 Note: The sensor requires a 3 minute warm-up time so this function is only available in tethered mode
> 
> | Parameter     | Direction | Type          | description
> |---------------|-----------|---------------|-------------
> |               | return    | void          |
> | sampletime_us | in        | unsigned long | measurement time for the sensor in μs

read_sht30
> Read and store values from the SHT30 temperature and humidity sensor.
>
> | Parameter     | Direction | Type          | description
> |---------------|-----------|---------------|-------------
> |               | return    | void          |
> | perform_store | in        | bool          | If true, the average of the sensor readings collected so far will be stored to RTC memory

read_hp303b
>Read and store values from the HP303B barometric pressure sensor.
>
> | Parameter     | Direction | Type          | description
> |---------------|-----------|---------------|-------------
> |               | return    | void          |
> | measure_temp  | in        | bool          | If true, the temperature will also be measured using this sensor and stored

read_vcc
> Read and store the ESP VCC voltage level (battery).
>
> | Parameter     | Direction | Type          | description
> |---------------|-----------|---------------|-------------
> |               | return    | void          |
> | perform_store | in        | bool          | If true, the average of the voltage readings collected so far will be stored to RTC memory

store_uptime
> Store the current uptime as an offset from RTC_MEM_DATA_TIMEBASE.
> 
> | Parameter     | Direction | Type          | description
> |---------------|-----------|---------------|-------------
> |               | return    | void          |

get_temp
> Return the current temperature.
>
> 🪧 Note: The temperature must have already been stored by calling read_sht30 or read_hp303b.  
> Otherwise, NAN will be returned.
>
> | Parameter     | Direction | Type          | description
> |---------------|-----------|---------------|-------------
> |               | return    | float         | The current temperature (°C)

get_humidity
> Return the current relative humidity.
>
> 🪧 Note: The humidity must have already been stored by calling read_sht30.  
> Otherwise, NAN will be returned.
>
> | Parameter     | Direction | Type          | description
> |---------------|-----------|---------------|-------------
> |               | return    | float         | The current relative humidity (%)

get_battery
> Return the current battery voltage.
>
> 🪧 Note: The battery voltage must have already been stored by calling read_vcc.  
> Otherwise, NAN will be returned.
>
> | Parameter     | Direction | Type          | description
> |---------------|-----------|---------------|-------------
> |               | return    | float         | The current battery voltage

#### SHT30
##### Description
![Sensors Component Overview](drawio/sensorsw_sht30_overview.png)  
The SHT30 driver provides a simple interface to retrieve sensor readings from one or more SHT30 sensors and helper functions for parsing the data returned.

##### Dependencies
| Component             | Interface Type     | Description
|-----------------------|--------------------|-------------
| Wiring                | function           | delay API
| TwoWire               | class              | I2C API
| lwip                  | preprocessor macro | ntohs

##### Configuration
There is no static configuration for this component.

##### Public API

###### Types and Enums
sht30_repeatability_t
> This enum provides options for the single-shot capture mode of the sensor.
> Higher repeatability will result in longer duration spent capturing the measurement.
>
> Enumerations:
> * SHT30_RPT_LOW
> * SHT30_RPT_MED
> * SHT30_RPT_HIGH


sht30_data_t
> This structure represents the data format returned by the sensor.
>
> Fields:
> * uint16_t temp
> * uint8_t temp_check
> * uint16_t humidity
> * uint8_t humidity_check


###### Functions
sht30_get
> Retrieve a measurement in single-shot mode.
>
> | Parameter | Direction | Type                  | description
> |-----------|-----------|-----------------------|-------------
> |           | return    | int                   | 0 for success, non-0 for failure
> | addr      | in        | uint8_t               | I2C Address for the sensor
> | type      | in        | sht30_repeatability_t | Measurement type
> | data_out  | out       | sht30_data_t*         | Data structure to return the measurement in - won't be modified on failure

sht30_parse_temp_c
> Convert the data reading into a float temperature value (°C)
> 
> | Parameter | Direction | Type         | description
> |-----------|-----------|--------------|-------------
> |           | return    | float        | Temperature in °C
> | data      | in        | sht30_data_t | Data structure with the measurement to convert

sht30_parse_temp_f
> Convert the data reading into a float temperature value (°F)
> 
> | Parameter | Direction | Type         | description
> |-----------|-----------|--------------|-------------
> |           | return    | float        | Temperature in °F
> | data      | in        | sht30_data_t | Data structure with the measurement to convert

sht30_parse_humidity
> Convert the data reading into a float humidity value (RH%)
> 
> | Parameter | Direction | Type         | description
> |-----------|-----------|--------------|-------------
> |           | return    | float        | Relative humidity
> | data      | in        | sht30_data_t | Data structure with the measurement to convert

sht30_check_temp
> Check the data reading temperature CRC
> 
> | Parameter | Direction | Type         | description
> |-----------|-----------|--------------|-------------
> |           | return    | bool         | True if CRC OK
> | data      | in        | sht30_data_t | Data structure with the measurement to check

sht30_check_humidity
> Check the data reading humidity CRC
> 
> | Parameter | Direction | Type         | description
> |-----------|-----------|--------------|-------------
> |           | return    | bool         | True if CRC OK
> | data      | in        | sht30_data_t | Data structure with the measurement to check

#### HP303B
##### Description
![Sensors Component Overview](drawio/sensorsw_hp303b_overview.png)  
The HP303B driver provides a class object to interface with an HP303B sensors via I2C or SPI.  
This component is provided as an external dependency developed by Lolin.

##### Dependencies
| Component             | Interface Type     | Description
|-----------------------|--------------------|-------------
| Wiring                | function           | delay API (and GPIO for SPI chipselect)
| TwoWire               | class              | I2C API
| SPI                   | class              | SPI API (unused in this project)

##### Configuration
There is no static configuration for this component.

##### Public API

###### Types and Enums
LOLIN_HP303B
> This class provides public interfaces for accessing the sensor.

###### Functions
> 🪧 Note: For brevity only used interfaces are shown here. For the complete API see https://github.com/wemos/LOLIN_HP303B_Library/blob/master/src/LOLIN_HP303B.h.

LOLIN_HP303B::LOLIN_HP303B
> Void Constructor

LOLIN_HP303B::~LOLIN_HP303B
> Destructor

LOLIN_HP303B::begin
> Function to initialize the sensor using default I2C interface.
>
> | Parameter    | Direction | Type    | description
> |--------------|-----------|---------|-------------
> |              | return    | void    |
> | slaveAddress | in        | uint8_t | I2C Address for the sensor (defaults to 0x77U)


LOLIN_HP303B::end
> Function to return the sensor to standby.
>
> | Parameter        | Direction | Type     | description
> |------------------|-----------|----------|-------------
> |                  | return    | void     |

LOLIN_HP303B::measureTempOnce
> Function to measure the temperature in one-shot mode with selected oversampling (2^n averages).
>
> | Parameter        | Direction | Type     | description
> |------------------|-----------|----------|-------------
> |                  | return    | int16_t  | 0 for success, non-0 for failure
> | result           | out       | int32_t& | Temperature result (°C)
> | oversamplingRate | in        | uint8_t  | Selected oversampling level 

LOLIN_HP303B::measurePressureOnce
> Function to measure the pressure in one-shot mode with selected oversampling (2^n averages).
>
> | Parameter        | Direction | Type     | description
> |------------------|-----------|----------|-------------
> |                  | return    | int16_t  | 0 for success, non-0 for failure
> | result           | out       | int32_t& | Pressure result (pascal)
> | oversamplingRate | in        | uint8_t  | Selected oversampling level 

#### Pulse2
### RTC Mem
### Persistent Storage
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

