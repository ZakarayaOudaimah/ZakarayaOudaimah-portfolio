# ZigBee Coordinator Overview

The ZigBee Coordinator (COO) is the central node in a ZigBee network. It is responsible for forming, managing, and maintaining the network.

## Responsibilities

- Form the ZigBee network
- Assign PAN ID
- Select operating channel
- Permit End Devices to join the network
- Maintain a list of joined devices
- Forward data from End Devices to the host MCU or system

## Network Topology

The Coordinator manages the following network topologies:

- Star
- Tree
- Mesh

## Communication

- Interface with MCU via UART
- Send/Receive AT commands to control the ZigBee module
- Handle device join and leave events
