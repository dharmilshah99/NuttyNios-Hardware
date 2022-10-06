# NuttyNios-Hardware

This repo contains all the source code for the hardware and firmware flashed onto the controller for the NuttyNios Game.

### Other repos:

- [NuttyNios Main Repo](https://github.com/dharmilshah99/NuttyNios)
- [NuttyNios Client](https://github.com/dharmilshah99/NuttyNios-Client/tree/main)
- [NuttyNios Server](https://github.com/dharmilshah99/NuttyNios-Server/tree/main)

## Features

- Fixed point FIR filter to reduce high frequency noise while maintaining low latency feel on controller.
- On-board LED & 7-SEG display which supports live feedback from game server which makes the game feel more interactive.

## FPGA to Server Communications

- Accelerometer is sampled at around 200Hz controlled by an interval timer.
- Data is fed through fixed-point filter to reduce noise & scaled. The filter applied and scale factor changes depending on difficulty.
- Filtered data is formatted & sent through nios2-terminal to client script.

## Server to FPGA Communications

- Server communicates with client script.
- Client inputs data into nios2-terminal through client script.
- FPGA receives data, parses it, then chages its state accordingly (eg. display text on 7-SEG, change led, change scale factor, etc...).
