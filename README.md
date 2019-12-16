# Internet of Spores
IOT Sensor Platform for ESP8266 Arduino
Specifically targeting Lolin D1 Mini.
![D1 mini board stackup](doc/stackup.png)

## Supported Sensors
* Temperature
  * SHT30
  * HP303B
* Relative Humidity
  * SHT30
* Barometric Pressure
  * HP303B
* Dust/Particle
  * PPD42NS

## Toolchain Setup and Library Dependencies

### ESP8266 Arduino
The ESP8266 Arduino toolchain is used to compile the code and provide basic
networking and multitasking services.
Project github: https://github.com/esp8266/Arduino

The toolchain can be installed via the Arduino board manager.
Instructions for this can be found [in their readme](https://github.com/esp8266/Arduino/blob/master/README.md).

#### Supported Version
I have thoroughly tested version 2.5.2. The latest version 2.6.2 compiles with
some caveats (mentioned below) and I anticipate will generally work without
issue.

#### Board Setup
Most board settings can be left at their defaults.
* Don't forget to select your board type and COM port
* Need to configure some flash for SPIFFS
  * v2.5.2: select Flash Size "4M (1M SPIFFS)"
  * v2.6.2: select option TBD

### WiFi Manager
There is a library dependency on WiFiManager by tzapu.
Project github: https://github.com/tzapu/WiFiManager

This can be installed via the library manager.
Please select a compatible version:
* v2.5.2: use WiFiManager version 0.14.0
* v2.6.2: use WiFiManager version 0.15.0-beta

### Lolin HP303B Driver
There is a library dependency on the Lolin HP303B driver.
Project github: https://github.com/wemos/LOLIN_HP303B_Library

This is not available in the library manager so it will have to be installed
manually. Clone or download the library from the link above. Installation
instructions can be found [in this guide](https://www.arduino.cc/en/Guide/Libraries).
