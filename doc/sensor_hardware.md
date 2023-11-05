# Sensor Electronic Architecture, Design, and Bill of Material

## Table of Contents
* [Overview](#overview)
  - [Block Diagram](#block-diagram)
  - [Description](#description)
  - [Restrictions](#restrictions)
  - [Communication Interfaces](#communication-interfaces)
  - [Power Management](#power-management)
* [Supporting Documentation](#supporting-documentation)
  - [Schematics](#schematics)
  - [Datasheets](#datasheets)
  - [Bill of Material](#bill-of-material)

---

## Overview
### Block Diagram
![Sensor HW Bloack Diagram](drawio/sensorhw_block_diagram.png)

### Description
The sensor nodes are composed of several off-the-shelf boards available from Lolin (formerly Wemos). These boards are compact and have a standard pin-header arrangement allowing them to be easily stacked together. They primarily work from a single 3.3V power rail and a shared I2C bus.  
![D1 Mini Stackup](photos/stackup.png)

The main microcontroller board is the D1 Mini which has the bulk of the modules needed for a working IOT development kit.  
![D1 Mini](photos/Lolin_D1_Mini.jpg)  
The onboard microcontroller is an Espressif ESP8266 which features a Tensilica Diamond Standard 106Micro 32-bit CPU core. The ESP8266 has built-in wireless communication in the form of an 802.11 b/g/n MAC with integrated transceiver, balun, and low-noise amplifier (LNA).

The additional supported sensors available from Lolin in this form-factor are:
* Lolin SHT30 Shield  
  ![SHT30](photos/SHT30.jpg)
  - Includes Sensiron SHT30 Humidity and Temperature Sensor
* Lolin Barometric Pressure Shield  
  ![HP303B](photos/HP303B.jpg)
  - Includes Hope Microelectronics HP303B Digital Pressure Sensor

These sensors also support temperature readout. (The SHT30 will be used in preference to the HP303B since it has higher precision.)

Additional devices outside of Lolin's system are supported:
* PPD42 Dust Sensor  
  ![PPD42NS](photos/PPD42NS.png)
  - This sensor outputs individual "low-pulse occupancy time" (LPO) signals for PM2.5 detections and PM1.0 detections.
  - The differentiation between PM2.5 and PM1.0 should be considered highly suspect and it is recommended to ignore the PM1.0 signal entirely.
  - This sensor is much larger than the D1 mini and requires a custom wiring harness to:
    + Wire the LPO outputs to D6 and D7
    + Wire D5 to ground as a presence detection pin
* Waveshare 1.9" E-Paper Display (EPD)  
  ![1.9in EPD](photos/EPD_1in9.jpg)
  - The display has an on-glass IST7134 120-segment EPD Driver
  - This display is available with or without a PCB and pin-header. A custom PCB [described in this project](../kicad/EPD_1in9/README.md) can adapt it to the D1 Mini standard header. If purchased with a PCB, the following notes apply:
    + Wire the Reset signal to D8 and the Busy signal to D7
    + Remove the R9 pullup from the Waveshare PCB (see [Usage Restrictions](#usage-restrictions) below)

### Restrictions
#### Usage Restrictions
The PPD42 and EPD cannot both be used simultaneously due to sharing the D7 GPIO pin. Additionally, the PPD42 requires 5V power so it can only be used in "tethered" mode where power is supplied via the USB port.

The Waveshare 1.9" E-Paper Display will not enter low-power mode during deep-sleep if there is a pull-up resistor on its reset pin. R9 can be removed from the Waveshare PCB that comes with these displays to allow reset to function correctly.  
![Location of R9 Resistor](photos/EPD_R9.jpg)  
Location of R9 Resistor (after removal)

> ⚠️ Warning:
> * The EPD may break when it is dropped or bumped on a hard surface. Handle with care.
> * Should the display break, do not touch the electrophoretic material.
> * In case of contact with electrophoretic material, wash with water and soap.

#### Operating Restrictions
| Device             | Property                 | Min | Max     | Units
|--------------------|--------------------------|-----|---------|------
| ESP8266            | Operating Temperature    | -40 | 125     | °C
|                    | IO Pin Current           |     | 12      | mA
| ME6211             | Operating Temperature    | -40 | 85      | °C
| CH340              | Operating Temperature    | -20 | 70      | °C
|                    | Voltage on Any Pin       |     | VCC+0.5 | V
| P25Q32SH           | Operating Temperature    | -40 | 85      | °C
|                    | Voltage on Any Pin       |     | 4.1     | V
| SHT30              | Operating Temperature    | -40 | 125     | °C
|                    | Recommended Temperature  | 5   | 60      | °C
|                    | Recommended Humidity     | 20  | 80      | %RH
|                    | Voltage on Any Pin       |     | VDD+0.5 | V
| HP303B             | Operating Temperature    | -40 | 85      | °C
|                    | Operating Pressure       | 300 | 1200    | hPa
|                    | Voltage on Any Pin       |     | 4       | V
| PPD42NS            | Operating Temperature    | 0   | 45      | °C
|                    | Operating Humidity       |     | 95      | %RH
|                    | Operating Voltage Ripple |     | 30      | mV
| Waveshare 1.9" EPD | Operating Temperature    | 0   | 50      | °C
|                    | Operating Humidity       |     | 70      | %RH

#### Storage Restrictions
| Device             | Property               | Min | Max | Units
|--------------------|------------------------|-----|-----|------
| ME6211             | Storage Temperature    | -55 | 125 | °C
| CH340              | Storage Temperature    | -55 | 125 | °C
| P25Q32SH           | Storage Temperature    | -65 | 150 | °C
| HP303B             | Storage Temperature    | -40 | 125 | °C
| PPD42NS            | Storage Temperature    | -30 | 60  | °C
| Waveshare 1.9" EPD | Storage Temperature    | 0   | 50  | °C
|                    | Storage Humidity       |     | 70  | %RH

### Communication Interfaces
#### GPIO
* Inputs
  - D5 (GPIO14) - Presence Detection (PPD42)
  - D6 (GPIO12) - PM2.5 LPO (PPD42)
  - D7 (GPIO13)
    + Busy (EPD)
    + PM1.0 LPO (PPD42)
* Outputs
  - D8 (GPIO15) - Reset (EPD)

#### I2C
* SHT30 - Address 0x45
* HP303B - Address 0x77
* Waveshare 1.9" EPD
  - Control - Address 0x3C
  - Data - Address 0x3D

#### SPI
SD/SPI port is connected to the flash memory.
This uses SPI CS0 and utilizes quad-spi for faster transfer rates.
Interface is compatible with Winbond W25Q flash memories.  
![SPI Flash](photos/SPI_NOR_flash.jpg)

#### UART
Debug output from ESP8266 is connected to CH340 USB-to-UART ASIC.
This is used for debugging, troubleshooting, and uploading new firmware.  
![CH340](photos/CH340.jpg)

#### WiFi
The ESP8266 has a built-in 802.11 b/g/n MAC with support for WPA2.

### Power Management
#### Power Options
The Sensor Nodes can be powered either from USB or Battery power.  
USB 5V power input is fed through an LDO regulator to produce a 3.3V supply. Alternately, an external 5V power supply can be fed into the 5V pin header.  
A 3.3V external power supply can be fed into the 3V3 pin header.

> 🪧 Note:  
> The PPD42NS Dust Sensor require a 5V supply and cannot be powered from a 3.3V external supply.

A LiFePO₄ battery is suitable as an external supply via the 3V3 pin header.  
All ICs are compatible with the typical voltage range of a LiFePO₄ battery, which is 2.8-3.6V. See the [table below](#input-voltage-ranges) for more details.
While it is possible for a LiFePO₄ to exceed 3.6V, it would very quickly fall down below 3.5V as it discharges -- in practice this does not pose a problem.

> 🪧 Note:  
> It is probably best to avoid powering the sensor node from USB while a battery is connected. The 3.3V LDO will attempt to "charge" the battery which could exceed the current output rating of the LDO or damage the battery.

#### Input Voltage Ranges

Device/Node            | Min | Nom | Max (V)
-----------------------|-----|-----|--------
ESP8266 VCC            | 2.5 | 3.3 | 3.6
ME6211 VIN             | 4.3 | 5.0 | 6.5
CH340 VCC              | 3.1 | 3.3 | 3.6
P25Q32SH VCC           | 2.3 | 3.3 | 3.6
Waveshare 1.9" EPD VDD | 1.9 | 3.0 | 3.6
SHT30 VDD              | 2.4 | 3.3 | 5.5
HP303B                 | 1.7 | 3.3 | 3.6
PPD42NS                | 4.5 | 5.0 | 5.5
 
#### EPD Power Consumption
The Waveshare 1.9" EPD supports partial updates as long as it has not been reset. However, if it is reset, the first update to the display after re-initializing it will trigger a full refresh.  
Leaving the display running while the ESP8266 is in deep sleep results in a noticeable power draw (~310 μA). There is therefore a trade-off between the extra time taken to refresh the display with the 310 μA continuous current draw.

Note that a full display refresh causes a double impact -- not only does it consume a large amount of current (mA), it takes several seconds (2-6 s depending on temperature). During this time, everything on the sensor node is drawing power, not just the display. So, this actively inhibits the sensor node from getting to sleep quickly.

Waveshare recommend doing a full refresh of the display every 3 minutes, so the power savings of a partial refresh would only be achieved on ⅔ of the wake-up cycles. This 3 minute suggestion is probably overly conservative when the display is primarily used at room temperature. Nonetheless, this tips the balance in favor of keeping the display in reset during deep-sleep.

---

## Supporting Documentation
### Schematics
* [Lolin D1 Mini (PDF)](https://www.wemos.cc/en/latest/_static/files/sch_d1_mini_v4.0.0.pdf)
* [Lolin SHT30 Shield (PDF)](https://www.wemos.cc/en/latest/_static/files/sch_sht30_v2.1.0.pdf)
* [Lolin HP303B Shield (PDF)](https://www.wemos.cc/en/latest/_static/files/sch_hp303b_v1.0.0.pdf)
* Waveshare 1.9" EPD
  - [D1 Mini Adapter PCB (PDF)](../kicad/EPD_1in9/EPD_1in9_schematic.pdf)
  - [Waveshare Driver Board (PDF)](https://www.waveshare.com/w/upload/3/35/1.9inch_Segment_e-Paper_Module01.pdf)

### Datasheets
* D1 Mini
  - [Espressif ESP8266 (PDF)](https://www.espressif.com/sites/default/files/documentation/0a-esp8266ex_datasheet_en.pdf)
  - SPI Flash - [Puya Semi P25Q32SH (PDF)](https://www.puyasemi.com/uploadfiles/2021/08/202108231525172517.pdf)
  - LDO
    + [MicrOne ME6211 (HTTP)](http://www.microne.com.cn/ProductDetail.aspx?id=4)
    + [LCSC Backup Link (PDF)](https://datasheet.lcsc.com/lcsc/1811131510_MICRONE-Nanjing-Micro-One-Elec-ME6211C33M5G-N_C82942.pdf)
  - USB-to-UART
    + [WinChipHead CH340](https://www.wch-ic.com/downloads/file/79.html)
    + [Mouser Backup Link (PDF)](https://www.mouser.com/datasheet/2/813/DS-16278-CH340E-1826268.pdf)
* [Sensiron SHT30 (PDF)](https://www.sensirion.com/resource/datasheet/sht3x-d)
* [HOPERF HP303B (PDF)](https://www.hoperf.com/api/downfile?title=&path=/uploads/HP303BDatasheet_1695352825.pdf)
* PPD42NS
  - [Seeed Studio Overview Brochure (PDF)](https://files.seeedstudio.com/wiki/Grove_Dust_Sensor/resource/Grove_-_Dust_sensor.pdf)
  - [Reverse Engineering by Tracy Allen (PDF)](https://web.archive.org/web/20141215160540/http://takingspace.org/wp-content/uploads/ShinyeiPPD42NS_Deconstruction_TracyAllen.pdf)
  - [PPD42NJ Product Specifications (PDF)](https://storage.googleapis.com/publiclab-production/public/system/images/photos/000/010/161/original/Spec_PPD42NJ_Eng_SPP13001V00_20130319.pdf)
* Waveshare 1.9" EPD
  - Waveshare Documentation
    + [Waveshare 1.9" EPD Specification (PDF)](https://files.waveshare.com/upload/5/53/1.9inch_Segment_e-Paper_Specification.pdf)
    + [Waveshare 1.9" EPD Wiki](https://www.waveshare.com/wiki/1.9inch_Segment_e-Paper_Module)
    + [Integrated Solutions Technology IST7134 (PDF)](https://www.waveshare.com/w/upload/b/bd/IST7134.pdf)
  - D1 Mini Adapter Board
    + [Civlux CF3527S (PDF)](https://www.cvilux.com/uploads/spec/101/681/SPCF053.pdf)

### Bill of Material
> 🪧 Note: Prices do not include shipping, assembly, or other ancillary costs.

#### Lolin Components
* Link to [Lolin Website](https://www.wemos.cc/en/latest/)
  - [D1 Mini](https://www.wemos.cc/en/latest/d1/d1_mini.html)
  - [SHT30 Shield](https://www.wemos.cc/en/latest/d1_mini_shield/sht30.html)
  - [HP303B Shield](https://www.wemos.cc/en/latest/d1_mini_shield/barometric_pressure.html)
* Link to [Lolin Store on Aliexpress](https://www.aliexpress.com/store/1100907255)
  - [D1 Mini](https://www.aliexpress.com/item/32529101036.html) - $3.70
  - [SHT30 Shield](https://www.aliexpress.com/item/2251832575822188.html) - $2.30
  - [HP303B Shield](https://www.aliexpress.us/item/2251832760893152.html) - $1.50

#### PPD42NS at Seeed Studio
* Link to [Seeed Studio Product Page](https://www.seeedstudio.com/Grove-Dust-Sensor-PPD42NS.html) - ~$10

#### EPD Adapter Board for D1 Mini

| Item                           | Count | ~Cost | ~Total | URL
|--------------------------------|-------|-------|--------|----
| PCB                            | 1     | $5    | $5     |
| PCB Solder Stencil (5mil)      | 1     | $2.50 | $2.50  |
| Waveshare 1.9" EPD Bare Screen | 1     | $7    | $7     | [Waveshare](https://www.waveshare.com/product/1.9inch-segment-e-paper.htm) / [AliExpress](https://www.aliexpress.us/item/3256804409831796.html)
| Civlux CF3527S                 | 1     | $0.30 | $0.30  | [Digikey CF3508WD0RM-NH](https://www.digikey.com/en/products/detail/cvilux-usa/CF3508WD0RM-NH/15792941)
| Capacitor SMD 0805 1000nF      | 1     | $0.10 | $0.10  | [Digikey CL21B105KBFNNNE](https://www.digikey.com/en/products/detail/samsung-electro-mechanics/CL21B105KBFNNNE/3886687)
| Capacitor SMD 1206 100nF 50V   | 2     | $0.10 | $0.20  | [Digikey C1206C104K5RAC7800](https://www.digikey.com/en/products/detail/kemet/C1206C104K5RAC7800/411248)
| Resistor SMD 1206 47kΩ         | 3     | $0.02 | $0.06  | Generic [Digikey Search](https://www.digikey.com/short/rm3qtzzb)
| Pinheader 0.1" Male            | 2     | $0.10 | $0.20  | Generic [Digikey Search](https://www.digikey.com/short/p78njrqb)
| **Total**                      |       |       | $15.40 |
