# LilyGo T5-ePaper-S3 Server Monitor

PlatformIO project for LilyGo T5-ePaper-S3 (4.7" e-paper, 960x540, ESP32-S3).

Based on: https://github.com/Xinyuan-LilyGO/LilyGo-EPD47

## Quick Start

1. Open in VS Code with PlatformIO

2. **Build:** Click **✓** (checkmark) in bottom left

3. **Upload:** Click **→** (arrow) in bottom left

   - Hold **middle button** (BOOT)
   - Press & release **reset button**
   - Release **reset**
   - Wait for upload notification
   - Release **middle button**

4. **Monitor:** Click **plug icon** in bottom left

## Current Project

- **Active:** `projects/screen_clear/` - Clears e-paper display
- **Switch projects:** Edit `src_dir` in `platformio.ini`

## Troubleshooting

### Memory Allocation Failed

- Ensure PSRAM is enabled in build flags
- Check that `-DBOARD_HAS_PSRAM` is present in platformio.ini

### Upload Issues

**Upload Failed or Not Connecting:**

1. Follow the button sequence carefully:
   - Hold **middle button** (BOOT)
   - Press and release **reset button**
   - Wait for upload notification
   - Release **middle button**
2. Ensure USB CDC is enabled
3. Check USB cable supports data transfer (not just power)
4. Try a different USB port
5. Make sure no other program is using the serial port

### Display Not Working
