# Server Software Architecture
This outline is currently a work-in-progress.

## Block Diagram
Show component decomposition (don't forget external components)

## External Dependencies
- Node Red nodes
- Interpreters
- Systemd (optional)
- Versions?

## Component Description (overview, dependencies, configuration, API and/or systemd service)
### Node-red Flows
### Node-red SOH Monitor
### Influx DB
### Grafana

## Dynamic Behavior
### Modes of Operation
- Normal Mode
- Node-red SOH Failure

### Sensor Processing
Describe incoming sensor readings, their storage in influxdb, their presentation in grafana.

### OTA Update
Describe Node-red FW Update process.
Describe Node-red Calibration/configuration update process.

### Node-red SOH
Describe SOH monitoring for Node-red

### Interrupt Behavior
- WiFi?
- PPD42 Pulse detection?

## Error Handling
- Grafana Sensor Alert (monitoring)
- Node-red packet parsing
- Node-red SOH monitoring
- Node-red debugging

