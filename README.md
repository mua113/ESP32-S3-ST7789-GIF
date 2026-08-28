# ESP32-S3 ST7789 GIF Player with WiFi Upload

ESP32-S3 N16R8 project with 1.54" ST7789 LCD display and WiFi-enabled GIF upload web interface.

## 📋 Features

- ✅ 240x240 ST7789 TFT LCD Display
- ✅ Animated GIF playback
- ✅ WiFi connectivity (FPT Bao Minh)
- ✅ Web-based GIF upload interface
- ✅ File management (list, delete)
- ✅ Drag & drop upload
- ✅ Real-time progress indicator
- ✅ SPIFFS storage

## 🔌 Pin Configuration

```
ESP32-S3  →  ST7789 LCD
───────────────────────
GND       →  GND
3.3V      →  VCC
GPIO12    →  SDA (MOSI)
GPIO13    →  SCL (SCK)
GPIO11    →  RST (Reset)
GPIO10    →  DC (Data/Command)
GPIO9     →  CS (Chip Select)
GND       →  BL (Backlight)
```

## 📦 Hardware Required

- ESP32-S3 N16R8 Dev Board
- ST7789 1.54" LCD Display (240x240)
- USB-C Cable for power & upload
- Breadboard & Jumper wires (optional)

## 🚀 Installation

### 1. Arduino IDE Setup

1. Install ESP32 board support (if not already installed)
2. Install required libraries:
   - `TFT_eSPI`
   - `AnimatedGIF`

3. Copy `User_Setup.h` to:
   ```
   Arduino/libraries/TFT_eSPI/User_Setup.h
   ```

4. Select board: **ESP32-S3 Dev Module**

5. Upload the sketch

### 2. PlatformIO Setup

```bash
platformio run -e esp32-s3-devkitc-1 --target upload
```

## 🌐 WiFi & Web Interface

### WiFi Connection
- **SSID:** FPT Bao Minh
- **Password:** 123456789
- **Auto-connect:** Yes (on boot)

### Access Web Interface

1. Open Serial Monitor (115200 baud)
2. Note the IP address displayed
3. Open browser: `http://<IP_ADDRESS>`
4. Upload GIF file via drag & drop interface

### Example URL
```
http://192.168.1.100
```

## 📁 File Management

- **Max file size:** 2MB
- **Storage:** SPIFFS (Flash)
- **Format:** GIF (animated)
- **Recommended size:** ≤240x240 pixels

## 💻 Web Upload Interface

Features:
- Drag & drop file upload
- File list with sizes
- Delete files from device
- Progress bar
- Status notifications
- Responsive design

## ⚡ Quick Start

1. **Prepare GIF:**
   - Use GIMP, ImageMagick, or online converters
   - Recommended: 240x240 or smaller
   - 256 colors or less (better performance)

2. **Convert to GIF:**
   ```bash
   # Using ImageMagick
   convert -resize 240x240 -delay 20 -loop 0 images/*.png animation.gif
   ```

3. **Upload via Web:**
   - Open `http://<ESP32_IP>`
   - Drag GIF file to upload area
   - Watch animation on LCD!

## 🔧 Troubleshooting

### WiFi Not Connecting
- Check SSID & password
- Ensure router is within range
- Check Serial Monitor for error messages

### GIF Not Loading
- Verify file is in SPIFFS
- Check file format (must be .gif)
- Use `loadFileList()` to see files

### Display Not Working
- Verify pin connections
- Check SPI communication (40 MHz)
- Monitor Serial for initialization messages

### Memory Issues
- Use smaller GIF files
- Reduce color palette to 256 colors
- Consider optimizing frame count

## 📊 Performance Tips

1. **Optimize GIF:**
   - Use online tools: ezgif.com, gifcompressor.com
   - Reduce frame count for faster playback
   - Use 256-color palette
   - Keep dimensions ≤240x240

2. **Improve Speed:**
   - Increase SPI frequency if stable
   - Use PSRAM if available (more buffer)
   - Reduce animation delay

## 🔌 Power Consumption

- Display: ~50-100mA (depends on brightness)
- WiFi active: ~80-150mA
- Total: ~150-250mA
- Recommended: 500mA+ USB power supply

## 📝 Serial Output Example

```
=== ESP32-S3 ST7789 GIF Player with WiFi ===
Display: 240x240
Initializing TFT...
SPIFFS mounted successfully!
Connecting to WiFi: FPT Bao Minh
WiFi connected! IP: 192.168.1.100
Starting Web Server...
Web server started!
Open http://192.168.1.100 in browser
Loading GIF...
GIF loaded: 240x240
```

## 📚 Library Links

- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)
- [AnimatedGIF](https://github.com/bitbank2/AnimatedGIF)
- [ESP32 Arduino](https://github.com/espressif/arduino-esp32)

## 📄 License

MIT License - Feel free to modify and use!

## 🤝 Support

For issues or questions:
1. Check Serial Monitor output
2. Verify hardware connections
3. Test with different GIF files
4. Clear SPIFFS and reload firmware

---

**Created for:** ESP32-S3 N16R8 + ST7789 1.54" LCD
**Project:** GIF Animation Player with WiFi Upload
**Last Updated:** 2025
