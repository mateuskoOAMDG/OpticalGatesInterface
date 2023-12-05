# Bench with optical gates

In this project, I am constructing a bench with optical gates to measure and investigate the movement of a ball. The finished assembly consists of a plate with a groove for the ball, on which the optical gates are placed and from the interface. Data is sent to the computer via the USB serial interface (typically the COMx port in Windows). Data collection is done by a *driver* programmed in Python.

## Optical gates

Optical gates are printed on a 3D printer. The gate models are in *stl* format and are in the **/3Dmodels** folder.
Optical gates contain simple electronics consisting of an infrared LED and a phototransistor. The signal from the phototransistor is shaped by the NE555 circuit. The gates are interconnected via a three-wire bus (VCC, GND, Signal). The first gateway connects to the *interface*, which is connected to the PC with a USB cable.

The wiring diagram is in the */schematics* directory

## Interface

The interface is a device that measures the time of appearance of signals from optical gates and sends them via USB to a PC. The heart of the interface is the Raspberry Pi Pico, which processes the received data and sends it to the PC via USB.

The wiring diagram is in the */schematics* directory.

The 3D printing model is in the folder */3Dmodels*

## The Driver

Driver is a program written in Python and serves for communication between the interface and the computer. The driver collects and saves the measured data from interface into a **.csv* text file.

## Table and Graph in Excel 

