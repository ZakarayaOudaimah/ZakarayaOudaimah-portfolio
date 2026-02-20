# ZigBee Coordinator AT Commands (Mesh Bee)

This document lists the AT commands used to control the Mesh Bee ZigBee module in Coordinator (COO) role.

---

## 1. Node Information Commands

| Command | Type   | Description                                                                                                            |
| ------- | ------ | ---------------------------------------------------------------------------------------------------------------------- |
| ATIF    | Action | Get node information (supported AT commands, firmware version, short address, MAC address, radio channel, ZigBee role) |
| ATLA    | Action | List all nodes in the network. Broadcasts topology query and prints responding nodes' short address, MAC, LQI, etc.    |

---

## 2. Data Transmit Commands

| Command | Type        | Description                                                               |
| ------- | ----------- | ------------------------------------------------------------------------- |
| ATTM    | Register RW | Set node's Tx Mode (0=broadcast, 1=unicast)                               |
| ATDA    | Register RW | Set unicast destination address (4-digit HEX)                             |
| ATBR    | Register RW | Set UART1 baud rate (0-4800, 1-9600, 2-19200, 3-38400, 4-57600, 5-115200) |

---

## 3. Network Formation Commands

| Command | Type        | Description                                                   |
| ------- | ----------- | ------------------------------------------------------------- |
| ATPA    | Register RW | Power-up action. 0=restore last network, 1=create new network |
| ATRS    | Action      | Re-scan network (Router/EndDevice)                            |
| ATLN    | Action      | List network scanned (Router/EndDevice)                       |
| ATJN    | Register RW | Join a network with specific index (Router/EndDevice)         |
| ATAJ    | Register RW | Auto join network scanned (1=enable, 0=disable)               |

---

## 4. OTA Commands (Coordinator)

| Command | Type        | Description                                        |
| ------- | ----------- | -------------------------------------------------- |
| ATOT    | Action      | Trigger OTA upgrade download to a destination node |
| ATOR    | Register RW | OTA block request rate (ms)                        |
| ATOA    | Action      | Abort OTA download for a specific node             |
| ATOS    | Action      | Query OTA status of a specific node                |

---

## 5. Other Commands

| Command | Type   | Description                       |
| ------- | ------ | --------------------------------- |
| ATRB    | Action | Reboot Mesh Bee (software reset)  |
| ATTT    | Action | Test functionality (factory test) |

---

### Notes

- Enter AT mode: `+++<CR>`
- Exit AT mode: `ATEX<CR>`
- UART default: 115200, 8 data bits, no parity, 1 stop bit
- All commands ignore case
