# GitHub Copilot Instructions for LilyGo T5-ePaper-S3 Server Monitor

## Project Overview

This is a PlatformIO project for the LilyGo T5-ePaper-S3 board with a 4.7" e-paper display (960x540 resolution). The board features an ESP32-S3 processor with OPI PSRAM and 16MB Flash.

## Development Environment

- **Platform**: PlatformIO (VS Code extension)
- **Framework**: Arduino
- **Board**: Custom T5-ePaper-S3 definition in `boards/` directory
- **Current Example**: `projects/screen_clear/main.cpp`

## Build & Upload Process

### Building

- Click the **checkmark (✓)** icon in bottom left corner of VS Code
- Or run: `C:\Users\stegi\.platformio\penv\Scripts\platformio.exe run`

### Uploading

- Click the **arrow (→)** icon in bottom left corner of VS Code
- **Critical Upload Sequence:**
  1. Hold down the **middle button** (BOOT button)
  2. While holding middle button, press and release the **reset button**
  3. Release the **reset button**
  4. Wait for upload notification to appear
  5. Release the **middle button**

### Monitoring Serial Output

- Click the **plug** icon in bottom left corner
- Or run: `C:\Users\stegi\.platformio\penv\Scripts\platformio.exe device monitor`
- Serial baud rate: 115200

## Project Structure

```
LilyGo-Server-Monitor/
├── boards/                      # Custom board definitions
│   └── T5-ePaper-S3.json       # ESP32-S3 board config
├── projects/                    # Application code
│   └── screen_clear/           # Current active project
│       └── main.cpp            # Main application file
├── src/                        # LilyGo-EPD47 library source
│   ├── epd_driver.h/.c         # E-paper driver
│   ├── utilities.h             # Pin definitions and utilities
│   ├── touch.h/.cpp            # Touch controller
│   ├── libjpeg/                # JPEG decoder
│   └── zlib/                   # Compression library
├── platformio.ini              # PlatformIO configuration
├── library.json                # Library metadata
└── library.properties          # Arduino library metadata
```

## Key Configuration Details

### platformio.ini

- `src_dir = projects/screen_clear` - Points to active project
- `boards_dir = boards` - Custom board definitions location
- `lib_extra_dirs = ${PROJECT_DIR}` - Includes src/ as library

### Board Definition (boards/T5-ePaper-S3.json)

- MCU: ESP32-S3
- Flash: 16MB QIO mode @ 80MHz
- PSRAM: OPI PSRAM enabled
- USB CDC on boot enabled

### Build Flags

- `-DBOARD_HAS_PSRAM` - Required for PSRAM support
- `-DARDUINO_USB_CDC_ON_BOOT=1` - USB CDC enabled
- `-DLILYGO_T5_EPD47_S3` - Board identifier

## Dependencies

- `lewisxhe/SensorLib @ 0.3.1` - Touch and sensor support
- `lennarthennigs/Button2 @ 2.3.2` - Button handling
- `bblanchon/ArduinoJson@^7.4.2` - JSON parsing
- Wire, SPI - Built-in libraries

## EPD Driver Usage

### Initialization

```cpp
epd_init();                    // Initialize e-paper driver
epd_poweron();                 // Power on display
epd_clear();                   // Clear display
epd_poweroff();                // Power off display (saves power)
```

### Framebuffer

```cpp
uint8_t *framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);  // Fill white
```

### Display Constants

- `EPD_WIDTH`: 960 pixels
- `EPD_HEIGHT`: 540 pixels
- Framebuffer size: EPD_WIDTH \* EPD_HEIGHT / 2 bytes (4-bit grayscale)

## Common Issues

### Build Errors

- Missing `epd_driver.h`: Ensure `src/` directory is present with library files
- Board not found: Check `boards/T5-ePaper-S3.json` exists
- PSRAM error: Verify `-DBOARD_HAS_PSRAM` in build flags

### Upload Errors

- Upload timeout: Follow the button sequence carefully
- Port not found: Check USB cable and port selection
- Permission denied: Close any other program using the serial port

### Runtime Errors

- Memory allocation failed: PSRAM not enabled or defective
- Display not responding: Check power supply and connections
- Touch not working: Ensure SensorLib is properly installed

## Hardware Buttons

- **Middle Button**: BOOT button (used for upload mode)
- **Reset Button**: Resets the ESP32-S3
- **Touch**: Capacitive touch controller (requires SensorLib)

## Pin Definitions

Located in `src/utilities.h`:

- EPD pins: Data bus, control signals
- Touch I2C: SDA/SCL pins
- Battery ADC: Battery voltage monitoring
- SD Card: SPI interface

## Code Style Guidelines

- Use Arduino framework functions
- Always call `epd_poweroff()` to save power when not using display
- Use `Serial.println()` for debugging (115200 baud)
- Use PSRAM for large allocations: `ps_calloc()`, `ps_malloc()`

## Switching Projects

To work on different examples, modify `platformio.ini`:

```ini
src_dir = projects/[project_name]
```

## Library Information

This project IS the LilyGo-EPD47 library itself (version 1.0.1). The library source code is in the `src/` directory and is based on the [epdiy](https://github.com/vroland/epdiy) drivers.

## Useful Commands

```powershell
# Build
C:\Users\stegi\.platformio\penv\Scripts\platformio.exe run

# Upload
C:\Users\stegi\.platformio\penv\Scripts\platformio.exe run --target upload

# Clean build
C:\Users\stegi\.platformio\penv\Scripts\platformio.exe run --target clean

# Monitor serial
C:\Users\stegi\.platformio\penv\Scripts\platformio.exe device monitor

# Build with verbose output
C:\Users\stegi\.platformio\penv\Scripts\platformio.exe run -v
```

## Repository

- Original library: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47 (esp32s3 branch)
- This project includes the full library source in `src/` directory
