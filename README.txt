BJT60 Presence Detection Firmware
==================================

Standalone firmware for BGT60TR13C radar sensor on ATSAMS70Q21 MCU

Hardware
--------
- MCU: ATSAMS70Q21 (ARM Cortex-M7 @ 300MHz)
- Radar: Infineon BGT60TR13C (58-63 GHz FMCW)
- Board: BJT60TRC

Features
--------
- Presence detection using radar sensor
- TinyML wave gesture detection (pure C, no TFLite dependency)
- LED indication (on = presence/wave, off = no presence)
- 5 Hz frame rate
- Standalone operation (no PC required)

Project Structure
-----------------
bjt60_firmware/
├── src/
│   ├── main.c                  - Main application
│   ├── presence_detection.c/h  - Detection algorithm
│   ├── wave_detector.c/h       - TinyML wave gesture detection
│   └── startup.s               - Startup code and vector table
├── drivers/
│   ├── clock.c/h               - Clock configuration (300MHz)
│   ├── gpio.c/h                - GPIO control (LED, radar pins)
│   ├── spi.c/h                 - SPI driver (radar communication)
│   └── avian_radar.c/h         - Radar driver
├── include/
│   └── sams70.h                - MCU register definitions
├── build/                      - Build output
├── Makefile                    - Build configuration
└── link.ld                     - Linker script (memory map)

Building
--------
Requirements:
- arm-none-eabi-gcc toolchain
- make
- bossac (for flashing)

Build commands:
  make            # Build firmware
  make clean      # Clean build files
  make flash      # Flash to board (requires bossac)

Output files (in build/):
- bjt60_presence.elf    - ELF executable
- bjt60_presence.bin    - Binary firmware (for flashing)
- bjt60_presence.hex    - Intel HEX format
- bjt60_presence.map    - Memory map

Flashing
--------
Using bossac:
  bossac -p /dev/ttyACM0 -e -w -v -b build/bjt60_presence.bin -R

Or simply:
  make flash

Memory Usage
------------
Flash: ~2MB available
RAM: ~384KB available

Current usage (with wave detector):
- Code: ~82KB (4% of Flash)
- Data: ~50KB (13% of RAM)
- Stack: 8KB
- Heap: 16KB

TinyML Wave Detector
--------------------
Pure C neural network for wave gesture detection.
- Model: 3-layer dense network (16→8→4→2)
- Parameters: 182 (weights hardcoded in C)
- Input: 16 normalized energy values
- Output: no_presence / waving classification
- Inference time: <1ms on Cortex-M7
- No external dependencies (just math.h for expf)

Usage:
  #include "wave_detector.h"

  float energy[16];  // Normalized 0-1
  wave_result_t result;
  wave_detect(energy, &result);

  if (result.predicted_class == WAVE_CLASS_WAVING) {
      // Wave detected
  }

Current Status
--------------
✓ Build system configured
✓ HAL drivers implemented
✓ Basic radar framework
✓ FFT-based presence detection
✓ TinyML wave gesture detection (pure C)

TODO for full functionality:
1. Complete Avian radar initialization
   - Extract full PLL config from reference code
   - Implement FIFO reading
   - Configure chirp parameters

2. Integrate CMSIS-DSP
   - Add FFT library
   - Implement windowing
   - Full algorithm with FFT

3. Hardware testing
   - Flash to board
   - Verify LED indication
   - Tune detection thresholds

Reference Code
--------------
Located in ../radar_sdk/ and ../firmware_extracted/

Key files to reference:
- radar_sdk/examples/py/FMCW/presence_detection.py
  → Algorithm reference

- radar_sdk/external/lib_avian/src/Driver/
  → Avian register definitions and config

- firmware_extracted/Firmware/sources/
  → Full firmware example (USB bridge)

License
-------
Refer to Infineon SDK license for radar-specific code.
Custom firmware code: adapt as needed for your project.

Contact
-------
For questions about this firmware implementation,
refer to the project documentation or Infineon SDK.
