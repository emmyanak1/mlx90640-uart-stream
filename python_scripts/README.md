# Visualize Captured Frames

This python script allows visualization of captured frames from MLX90640 UART. 

## How to 

- run locally on a Mac while STM32 is connected via USB to UART cable
- identify the UART port through terminal command <screen /dev/tty.usbmodem-XXXXX>
- update PORT variable in script to match 
- if PORT is being utilized find & kill other process:
	- lsof <screen /dev/tty.usbmodem-XXXXX>
- push B1 on STM32 nucleo board to capture 3 frames 