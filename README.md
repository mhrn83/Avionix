# Avionix

## Table of Contents
- [Introduction](#introduction)
- [Objectives](#objectives)
- [Functional Requirements](#functional-requirements)
- [Serial Communication](#serial-communication)
- [Graphical Display of Sensitive Data](#graphical-display-of-sensitive-data)
- [Display of All Data in Two Tables](#display-of-all-data-in-two-tables)
- [Settings Page](#settings-page)
- [Data Storage in Excel Format](#data-storage-in-excel-format)
- [User Testing](#user-testing)
- [Project Demo](#project-demo)
- [Installation and Usage](#installation-and-usage)

## Introduction
Anomalies in aircraft engine performance can lead to catastrophic accidents. To prevent such incidents, pre-flight engine performance testing is conducted using multiple sensors. These sensors transmit data to the **Avionix** software, which displays the data graphically and stores it.

## Objectives
The objectives of this project are to receive data from a serial port based on a defined protocol, display the data, and store it. Displaying critical data, such as engine temperature, provides human operators with essential information about the current state of the engine under test. Storing the data allows for further analysis of the engine's condition and comparison of its performance under different conditions.

## Functional Requirements
The project consists of four main components:
1. Serial communication with sensor processors.
2. Graphical display of critical information (e.g., light warnings, text, gauges).
3. Display of all information in two distinct tables.
4. Settings page including serial port configurations and start/stop test commands.
5. Storage of all data in Excel format.

## Serial Communication
Data is received via a serial port with the following specifications:
- BaudRate: 115200
- Parity: OddParity
- StopBit: OneStop

The communication protocol involves messages with the following structure:

| Field         | Byte Index | Value/Length  |
|---------------|-------------|---------------|
| HEADER        | 0-3         | 0xA5 (4 bytes)|
| MSG COUNTER   | 4           | UINT8 (1 byte)|
| ID NUMBER     | 5           | N (UINT8, 1 byte)|
| ID 1          | 6           | UINT8 (1 byte)|
| RESERVE       | 7           | 0 (1 byte)    |
| DATA 1        | 8-11        | UINT32 (4 bytes)|
| FACTOR 1      | 12-15       | UINT32 (4 bytes)|
| ...           | ...         | ...           |
| ID N          | ...         | UINT8 (1 byte)|
| RESERVE       | ...         | 0 (1 byte)    |
| DATA N        | ...         | UINT32 (4 bytes)|
| FACTOR N      | ...         | UINT32 (4 bytes)|
| CHECKSUM      | ...         | UINT16 (2 bytes)|
| FOOTER        | ...         | 0x55 (1 byte) |

## Graphical Display of Sensitive Data
The following sensitive data will be displayed using circular gauges:
- Oil Pressure
- Oil Temperature
- Fuel
- Torque
- Motor Speed

Warning lights will indicate errors in sensor readings.

## Display of All Data in Two Tables
Sensor error data will be displayed in one table, and all other data will be displayed in another table. Each row in the table will include the data name and its value.

## Settings Page
The settings page will include fields for serial port address, baud rate, parity, and stop bit configurations. Users will start the test by pressing the "Start" button, which opens the serial port and begins parsing and displaying data. The "Stop" button will close the serial port, stop data reception and display, and close the file storing the data.

## Data Storage in Excel Format
When the user presses the "Start" button, a new file will be created, and all received data will be stored in Excel format. Each row of the Excel file will represent data received from one message. Pressing the "Stop" button will save and close the file.

## User Testing
Sensory program, sends engine sensor data over the serial port for desktop testing. Two virtual ports will be created, and each program will connect to one of the virtual serial ports. **SENSORY** will begin sending data for testing. This program is provided in sensory folder.

## Project Demo
Click To Watch Full Video on Youtube :
[![](https://github.com/user-attachments/assets/b97138d1-3332-45e3-b2c2-8ee71b3ad3dd)
](https://www.youtube.com/watch?v=Hr5jNGkqMNg)

## Installation and Usage

1. **Clone this repository**:

   ```bash
   git clone https://github.com/mhrn83/Avionix.git

2. **Run programs**:

   run the program in Qt 5.13 or upper versions. Also you need to run Sensory.exe
   
3.  **Make virtual serial ports**:
   There are lots of free software to carry this out. For example I use [**Virtual Serial Port Tools**](https://freevirtualserialports.com/) to make a pair of serial port connections.
4. **Test the software**:
   Make arbitrary changes with Sensory app and see the results in main app.
