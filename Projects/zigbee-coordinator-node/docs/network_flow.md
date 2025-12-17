# ZigBee Coordinator Network Flow

This document explains the workflow of the ZigBee Coordinator (COO) and how it manages the network and End Devices.

## 1. Power On

- MCU powers up
- ZigBee module initialized
- UART communication established between MCU and ZigBee module

## 2. Network Formation

- Coordinator sets PAN ID
- Selects channel
- Starts network formation
- Coordinator waits for End Devices to join

## 3. Permit Join

- Coordinator enables "permit join" mode
- End Devices send join requests
- Coordinator authenticates and adds devices to the network table

## 4. Device Communication

- Coordinator receives data from End Devices
- Data processed or forwarded to host MCU
- Coordinator can send commands back to End Devices

## 5. Network Maintenance

- Monitor joined devices
- Handle device leave events
- Retry failed communication
- Keep network stable

## 6. Logging

- Log join, leave, and data events
- Store logs locally for debugging or analytics
