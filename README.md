# MLX90640 UART Streaming with STM32

This project streams thermal image data from the MLX90640 thermal sensor using an STM32 microcontroller (Nucleo-F446RE). The data is sent over UART to a host PC, where it is visualized using Python.

## Features

- Button-triggered three-frame capture modes
- UART data transmission using quantized binary format to reduce payload. 
- Frame visualization in real-time on the host computer using `matplotlib`

## Folder Structure

- `Core/` and `Drivers/` - STM32 HAL code
- `.ioc` file - STM32CubeMX project configuration
- `python_scripts/` - Python code for visualization 

## Hardware

- STM32 Nucleo-F446RE
- MLX90640 thermal sensor board 
- USB or UART serial connection to Mac (python script only works on Mac)

## Capture Modes

Define `SINGLE_CAPTURE_MODE` in `main.c` to enable triggered 3-frame capture via button B1. You can define `SREAM_MODE` for continuous streaming. 

UART baud rate is set to 115200 by default but may be increased for streaming mode in the future. 

You must set PORT in the visualize_capture.py script to the correct UART port on your computer. 
