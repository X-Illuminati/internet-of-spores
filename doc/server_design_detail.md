# Server Software Design
This outline is currently a work-in-progress.

## Table of Contents

* [Module Overview](#module-overview)
* [Project Configuration](#project-configuration)
* [Module Description](#module-description)
  - [Node-RED Flows](#node-red-flows)
    + [Main Entry](#main-entry)
    + [Handle Sensor Readings](#handle-sensor-readings)
    + [Update Firmware](#update-firmware)
    + [Update Config](#update-config)
    + [State of Health](#state-of-health)
  - [Node-RED SOH Monitor](#node-red-soh-monitor)
* [Debugging and Unit Testing](#debugging-and-unit-testing)

---

## Module Overview

- Block diagram / class diagram
- Show dependency between files (configuration, flows, etc)
- Description/overview

---

## Project Configuration

- Describe configuration/build options
- Describe build system/toolchain
- Systemd units

---

## Module Description

(configuration options, flow charts, fault tolerance, notes, dependencies)

### Node-RED Flows

The flows are found in [flows.json](../node-red/flows.json) and are split into
the following tabs:
+ [Main Entry](#main-entry)
+ [Handle Sensor Readings](#handle-sensor-readings)
+ [Update Firmware](#update-firmware)
+ [Update Config](#update-config)
+ [State of Health](#state-of-health)

#### Main Entry

![Flow Screenshot](screenshots/flows_detail_entry.png)

#### Handle Sensor Readings

![Flow Screenshot](screenshots/flows_detail_handle_readings.png)

#### Update Firmware

![Flow Screenshot](screenshots/flows_detail_update_firmware.png)

#### Update Config

![Flow Screenshot](screenshots/flows_detail_update_config.png)

#### State of Health

![Flow Screenshot](screenshots/flows_detail_soh.png)

### Node-red SOH Monitor

---

## Debugging and Unit Testing

- Debugging
- console.log
- Fault tolerance
- run soh monitor from the command line

---

## Future Improvements

-find some way to make the tabs more modular so they can be included into an existing set of flows or so they can be expanded upon but still have an easy upgrade path