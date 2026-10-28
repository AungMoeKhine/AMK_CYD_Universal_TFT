# AMK_CYD_Universal_TFT

[![Arduino Compatible](https://img.shields.io/badge/Arduino-Compatible-00979C.svg?style=flat-square&logo=arduino)](https://www.arduino.cc/)
[![ESP32 Core](https://img.shields.io/badge/ESP32-v2.x%20%7C%20v3.x-E7352C.svg?style=flat-square&logo=espressif)](https://github.com/espressif/arduino-esp32)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg?style=flat-square)](LICENSE)
[![Hardware](https://img.shields.io/badge/CYD-2.4%22%20%7C%202.8%22%20%7C%203.2%22%20%7C%203.5%22%20%7C%204.0%22-brightgreen.svg?style=flat-square)](https://github.com/aungmoekhine/AMK_CYD_Universal_TFT)

**Universal standalone TFT, XPT2046 resistive touch, and off-screen sprite driver for the entire ESP32 Cheap Yellow Display (CYD) family.**

Zero configuration required. No editing User_Setup.h, no hunting down obscure pinouts, and no recompiling for different screen variants. Works out of the box on **2.4", 2.8", 3.2", 3.5", and 4.0"** CYD resistive touch boards with runtime auto-detection.

---

## ⚡ Why AMK_CYD_Universal_TFT?

The popular **ESP32 Cheap Yellow Display (CYD)** boards are incredible value for IoT dashboards and maker projects, but getting them working with standard libraries like TFT_eSPI is notoriously frustrating:
* CYD boards ship with **multiple display controllers** (ILI9341, ST7789, ST7796).
* They use **different screen resolutions** (240x320 vs 320x480).
* Backlight wiring differs across revisions (**GPIO21** on 2.8" vs **GPIO27** on 2.4", 3.2", 3.5", 4.0").
* Color polarities differ (3.2" ST7789 requires inversion, but 2.4" ST7789 does not).
* The **XPT2046 touch controller** is wired completely differently:
  * 2.8" boards wire touch to dedicated bit-bang pins.
  * 2.4", 3.2", 3.5", & 4.0" boards wire touch to the shared HSPI display bus.
  * 2.4" boards have X/Y ADC channels physically swapped.
* Onboard RGB LED pins change between revisions (**GPIO4** vs **GPIO22** for Red; **GPIO16/17** swapped on 024 and 035 models).
* Standard TFT_eSPI requires manually modifying files inside the Arduino `libraries/` directory every single time you change boards.

**AMK_CYD_Universal_TFT solves all of this automatically at runtime.**

---

## 🚀 Key Features

* 🔍 **Runtime Hardware Auto-Probing**:
  * Detects board wiring automatically via passive pull-down probing on the backlight pins.
  * Probes panel identity registers (SDO on GPIO12) at boot to identify ILI9341, ST7789, or ST7796.
  * **Smart 2.8" Silent-MISO Detection**: Probes the onboard LDR (GPIO 34) to cleanly differentiate between Dual-USB ST7789 boards and ELEGOO ILI9341 boards without requiring manual config files.
  * Detects specific board models (`display.modelName()`) and automatically configures color inversion, subpixel order, and touch axis mapping.
* 📦 **100% Self-Contained & Drop-In Compatible**:
  * Provides a complete, standalone `TFT_eSPI` class implementation.
  * Does **not** require installing Bodmer's TFT_eSPI library.
  * Drop-in compatible with existing TFT_eSPI drawing calls (`drawString`, `fillRoundRect`, `fillCircle`, `drawTriangle`, etc.).
* 🎨 **Full Off-Screen Sprite Engine (TFT_eSprite)**:
  * **16-bit color** (RGB565 full depth)
  * **8-bit color** (RGB332 low memory)
  * **4-bit color** (16-color indexed palette: run full 240x240 analog gauges in only ~28 KB RAM!)
  * Supports transparent color keying and push-to-display / push-to-sprite.
* 👆 **Universal XPT2046 Resistive Touch Driver**:
  * Automatically switches between dedicated bit-banging and hardware HSPI SPI bus arbitration.
  * Handles native coordinate rotation and axis swapping transparently.
  * Seamless `tft.getTouch(&x, &y)` and `touch.read(x, y)` support.
* 💾 **Isolated NVS Touch Calibration**:
  * Built-in interactive 4-corner calibration tool.
  * Stores calibrations in distinct NVS namespaces per board model, preventing settings on one board variant from corrupting another.
  * Every sketch loads your tuned calibration on boot with `touch.loadCalibration()`.
* 🛡️ **Clean SPI Separation (SD Card Friendly)**:
  * Display and touch run on **SPI2 (HSPI)**.
  * Leaves **SPI3 (VSPI: GPIO 18, 19, 23, 5)** completely open for high-speed micro-SD card operations without bus conflict!
* 🛑 **Compile-Time Failsafe Protection**:
  * Asserts compile-time warnings if PSRAM is accidentally enabled or flash is set > 4 MB, preventing boot loops and GPIO16/17 pin conflicts.

---

## 📋 Supported Hardware Matrix

| CYD Model | Screen Size | Resolution | Controller | Touch Bus | Backlight Pin | Red LED Pin |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|
| **ESP32-2432S028R** (Dual-USB: Type-C + Micro-USB + LDR) | 2.8" | 240x320 | ST7789 | Dedicated (Bit-bang) | GPIO 21 | GPIO 4 |
| **ESP32-2432S028 / ELEGOO 2.8"** (Single USB-C, no LDR) | 2.8" | 240x320 | ILI9341 | Dedicated (Bit-bang) | GPIO 21 | GPIO 4 |
| **ESP32-32E 2.8"** (USB-C + Batt) | 2.8" | 240x320 | ILI9341 / ST7789 | Dedicated (Bit-bang) | GPIO 21 | GPIO 22 |
| **ESP32-024** (2.4" USB-C + Batt) | 2.4" | 240x320 | ST7789 / ILI9341 | Shared HSPI Bus (X/Y Swap) | GPIO 27 | GPIO 4 |
| **ESP32-32E 3.2"** | 3.2" | 240x320 | ST7789 | Shared HSPI Bus | GPIO 27 | GPIO 22 |
| **ESP32-035** (3.5" USB-C + Batt) | 3.5" | 320x480 | ST7796 | Shared HSPI Bus (X/Y Mirror) | GPIO 27 | GPIO 4 |
| **ESP32-32E 3.5"** | 3.5" | 320x480 | ST7796 | Shared HSPI Bus | GPIO 27 | GPIO 22 |
| **ESP32-32E 4.0"** | 4.0" | 320x480 | ST7796 | Shared HSPI Bus | GPIO 27 | GPIO 22 |

---

## 🛠️ Recommended Arduino IDE Settings

When compiling for any CYD board, use the following settings in the Arduino IDE:

* **Board**: ESP32 Dev Module
* **Upload Speed**: 921600
* **CPU Frequency**: 240MHz (WiFi/BT)
* **Flash Frequency**: 80MHz
* **Flash Mode**: QIO
* **Flash Size**: 4MB (32Mb) *(Do NOT select 8MB or 16MB)*
* **Partition Scheme**: Default 4MB with spiffs or Huge APP (3MB No OTA/1MB SPIFFS)
* **PSRAM**: Disabled *(Do NOT enable PSRAM; CYD boards do not have external PSRAM, and enabling it locks up GPIO16 & GPIO17)*

---

## 📥 Installation

### Method 1: Arduino IDE (ZIP Install)
1. Download this repository as a `.ZIP` file from GitHub (`Code` -> `Download ZIP`).
2. Open **Arduino IDE**.
3. Navigate to **Sketch** -> **Include Library** -> **Add .ZIP Library...**
4. Select the downloaded `AMK_CYD_Universal_TFT.zip`.

### Method 2: Git Clone
Clone directly into your Arduino libraries directory:
```bash
# Windows (PowerShell)
cd C:\Users\PSA\Documents\Arduino\libraries
git clone https://github.com/aungmoekhine/AMK_CYD_Universal_TFT.git

# Linux / macOS
cd ~/Documents/Arduino/libraries
git clone https://github.com/aungmoekhine/AMK_CYD_Universal_TFT.git
```

### Method 3: PlatformIO (`platformio.ini`)
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
upload_speed = 921600
build_flags = 
    -D CORE_DEBUG_LEVEL=0
lib_deps = 
    https://github.com/aungmoekhine/AMK_CYD_Universal_TFT.git
```

---

## 💻 Quick Start Examples

### 1. Basic Display & Touch

```cpp
#include <Arduino.h>
#include <AMK_CYD_Universal_TFT.h>

// Global driver objects
AMK_CYD_display display;
AMK_CYD_touch   touch;
TFT_eSPI        tft;

void setup() {
  Serial.begin(115200);

  // 1. Initialize display (auto-detects board wiring, panel, and model)
  tft.init();
  tft.setRotation(1);       // 0 = Portrait, 1 = Landscape, 2 = Flip Port, 3 = Flip Land
  tft.setBacklight(100);    // 0 to 100% PWM brightness

  // 2. Initialize touch and sync rotation
  touch.begin(display);
  touch.setRotation(1);

  // 3. Load saved calibration from flash (if available)
  touch.loadCalibration();
  touch.loadFilters();

  // 4. Draw UI
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Hello from AMK_CYD_Universal_TFT!", tft.width() / 2, 40, 2);

  char info[96];
  snprintf(info, sizeof(info), "Model: %s | Panel: %s (%dx%d)", 
           display.modelName(), display.panelName(), tft.width(), tft.height());
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(info, tft.width() / 2, 70, 2);
}

void loop() {
  uint16_t x, y;
  // Read touch input
  if (tft.getTouch(&x, &y)) {
    tft.fillCircle(x, y, 3, TFT_YELLOW);
  }
  delay(5);
}
```

---

### 2. Off-Screen Sprite Animation (Flicker-Free)

```cpp
#include <Arduino.h>
#include <AMK_CYD_Universal_TFT.h>

AMK_CYD_display display;
AMK_CYD_touch   touch;
TFT_eSPI        tft;

TFT_eSprite sprite = TFT_eSprite(&tft);

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  // Create an 80x80 16-bit RGB565 sprite in RAM
  sprite.setColorDepth(16);
  sprite.createSprite(80, 80);
}

void loop() {
  static int angle = 0;
  angle = (angle + 5) % 360;

  // Render into off-screen buffer
  sprite.fillSprite(TFT_NAVY);
  sprite.drawRect(0, 0, 80, 80, TFT_CYAN);
  
  float rad = angle * 0.0174533f;
  int xTip = 40 + (int)(cos(rad) * 30.0f);
  int tipY = 40 + (int)(sin(rad) * 30.0f);
  sprite.drawLine(40, 40, xTip, tipY, TFT_YELLOW);

  // Push buffer to display in one fast burst
  sprite.pushSprite(120, 80);

  delay(20);
}
```

---

## 🎯 Touch Calibration & Noise Filters

Resistive touch screens vary physically between individual manufacturing batches. This library includes a dedicated calibration app in `examples/TouchCalibration/TouchCalibration.ino`:

1. Open **File** -> **Examples** -> **AMK_CYD_Universal_TFT** -> **TouchCalibration**.
2. Upload to your CYD board.
3. Tap each of the 4 crosshairs using a stylus or precise tap.
4. Adjust the **Pressure** and **Jump Filter** sliders to eliminate jitter.
5. Tap **SAVE** in the top right.
6. The calibration is permanently written into the ESP32's **NVS Flash** under that board's dedicated namespace.
7. Now every sketch calling `touch.loadCalibration()` will automatically use your tuned calibration!

---

## 📌 Pinout & Bus Mapping Reference

### Display & Touch Bus (HSPI - SPI2)
* **SCK / Clock**: GPIO 14
* **MOSI**: GPIO 13
* **MISO / SDO**: GPIO 12
* **TFT CS**: GPIO 15
* **TFT DC**: GPIO 2
* **Backlight**: GPIO 21 (2.8" Own-Touch) or GPIO 27 (2.4", 3.2", 3.5", 4.0" Shared-Touch) — *Query via `display.backlightPin()`*
* **Touch CS**: GPIO 33
* **Touch IRQ**: GPIO 36

### MicroSD Card Bus (VSPI - SPI3 - Dedicated)
* **SD SCK**: GPIO 18
* **SD MISO**: GPIO 19
* **SD MOSI**: GPIO 23
* **SD CS**: GPIO 5

### Onboard Peripherals
* **RGB LED**:
  * **Red**: GPIO 4 or GPIO 22 — *Query via `display.redLedPin()`*
  * **Green**: GPIO 16 (17 on ESP32-024 / ESP32-035) — *Query via `display.greenLedPin()`*
  * **Blue**: GPIO 17 (16 on ESP32-024 / ESP32-035) — *Query via `display.blueLedPin()`*
* **LDR (Light Dependent Resistor)**: GPIO 34 (ADC1_CH6)
* **Buzzer / Audio**: GPIO 26 (DAC)

---

## ❓ Frequently Asked Questions (FAQ)

### Q: Screen appears mirrored horizontally
The library automatically differentiates between 2.8" ILI9341 boards (e.g. ELEGOO 2-Pack) and ST7789 boards (Dual-USB). If you ever need to manually toggle horizontal mirroring for a custom panel, call:
```cpp
tft.setMirrorX(true);  // or false to unmirror
```

### Q: Can I manually force a specific display panel?
Yes. Call `setPanel()` before `tft.init()`:
```cpp
tft.setPanel(CYDR_PANEL_ILI9341); // Force ILI9341
// or tft.setPanel(CYDR_PANEL_ST7789); // Force ST7789
tft.init();
```

### Q: Colors look inverted (White is Black, Red is Cyan)
The library automatically handles hardware inversion via `_needsInversion()`. If using a rare non-standard replacement panel, call `tft.invertDisplay(true);` or `tft.invertDisplay(false);`.

### Q: Red and Blue colors are swapped (BGR vs RGB)
Call `tft.setBGR(true);` or `tft.setBGR(false);` to flip the subpixel order.

### Q: Touch coordinates are misaligned
Upload and run the `TouchCalibration` example from this library, follow the crosshair prompts, and tap **SAVE**.

### Q: Board boot loops with CORRUPT HEAP or PSRAM errors
In your Arduino IDE menu, make sure **Tools** -> **PSRAM** is set to **Disabled**. CYD boards do not have external PSRAM chips.

---

## 👤 Author & Acknowledgments

* **Author**: Aung Moe Khine ([@aungmoekhine](https://github.com/aungmoekhine))
* **Architecture & Inspiration**: Built upon hardware probing techniques and high-speed driver routines from the ESP32 open-source maker community.
* **License**: MIT License — free for hobbyist and commercial projects.
