# AMK_CYD_Universal_TFT

[![Arduino Compatible](https://img.shields.io/badge/Arduino-Compatible-00979C.svg?style=flat-square&logo=arduino)](https://www.arduino.cc/)
[![Release](https://img.shields.io/badge/Release-v1.6.3-blue.svg?style=flat-square)](https://github.com/aungmoekhine/AMK_CYD_Universal_TFT/releases)
[![ESP32 Core](https://img.shields.io/badge/ESP32-v2.x%20%7C%20v3.x-E7352C.svg?style=flat-square&logo=espressif)](https://github.com/espressif/arduino-esp32)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg?style=flat-square)](LICENSE)
[![Hardware](https://img.shields.io/badge/CYD%20%2B%20S3%20RGB-2.4%22%20to%207.0%22-brightgreen.svg?style=flat-square)](https://github.com/aungmoekhine/AMK_CYD_Universal_TFT)

**Universal standalone TFT, touch, sprite, and visual GUI driver for the entire ESP32 Cheap Yellow Display (CYD) family (resistive & capacitive) and ESP32-S3 Parallel RGB displays.**

Zero configuration required. No editing User_Setup.h, no hunting down obscure pinouts, and no recompiling for different screen variants. Works out of the box on **2.4", 2.8", 3.2", 3.5", and 4.0"** CYD resistive and capacitive touch boards with runtime auto-detection, as well as **4.3", 5.0", and 7.0"** ESP32-S3 parallel RGB displays with GT911 capacitive multi-touch.

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

* 🪄 **1-Line Universal Hardware Auto-Setup (`tft.autoSetup()`) [NEW in v1.6.0]**:
  * Eliminates 30–50 lines of duplicate hardware initialization boilerplate across all sketches!
  * Single call (`tft.autoSetup(3)`) automatically probes and configures:
    * **Display Panel & Bus**: Calls `init()`, sets rotation, enforces `setSwapBytes(true)`, and verifies MADCTL color mapping.
    * **Backlight Pin & 8-Bit PWM**: Dynamically detects GPIO 21 vs GPIO 27 with zero hardcoded pins and zero fallback pins.
    * **LDR Ambient Light Sensor**: Detects GPIO 34 role, configures input, and primes the 10-read moving average filter before the panel turns on.
    * **Active-LOW RGB LED**: Detects Red (GPIO 4 vs 22), Green, and Blue pins, initializes them OFF (`HIGH`), and runs an instant R-G-B boot sequence.
    * **Touch Subsystem**: Probes Resistive (XPT2046) vs Capacitive (GT911/CST820), syncs rotation, and auto-loads persistent NVS calibration.
    * **Serial Diagnostic Audit**: Automatically prints a full hardware specification report at 115200 baud.
* 🎯 **Adaptive Multi-Point Statistical Touch Calibration (`tft.autoCalibrate()`) [NEW in v1.6.0]**:
  * No separate calibration sketch required! Built directly into the library.
  * **Zero boot delay (< 1 ms)** if calibration is already stored in NVS flash.
  * **Zero boot delay (0 ms)** if capacitive touch (CST820 / GT911) is detected.
  * **Adaptive 12-to-30 Point Grid ($4 \times 3$ to $6 \times 5$)**: Runs multi-sample ADC averaging with raw jitter rejection (`JITTER_RAW = 130`, `MIN_SAMPLES = 8`), statistical 2D linear regression (`linfit`), RMS error verification (`ACCEPT_RMS <= 6.0 px`), and hardware mirroring compensation.
  * **Automatic NVS Flash Persistence**: Commits calibrated boundaries and filter parameters to ESP32 NVS flash so subsequent boots load instantly.
  * **Hardware BOOT Pin Override**: Holding the CYD `BOOT` button (GPIO 0) at startup forces re-calibration on demand.
* 💡 **Built-In Ambient Light Auto-Dimming (`tft.adjustBrightness()`) [NEW in v1.6.0]**:
  * Execute `tft.adjustBrightness()` as the first line of `void loop()` for instant, flicker-free auto-dimming with an integrated 10-sample moving average buffer and 4-unit hysteresis deadband.
* 🚥 **Built-In Universal RGB LED Helpers [NEW in v1.6.0]**:
  * Drive the onboard RGB LED effortlessly without worrying about Active-LOW polarity or pin variants: `tft.setRgbLed(r, g, b)`, `tft.turnLedOff()`, `tft.flashLed()`.
* 🔍 **Runtime Hardware Auto-Probing**:
  * Detects board wiring automatically via passive pull-down probing on the backlight pins.
  * Probes panel identity registers (SDO on GPIO12) at boot to identify ILI9341, ST7789, or ST7796.
  * **Smart 2.8" Silent-MISO Detection**: Probes the onboard LDR (GPIO 34) to cleanly differentiate between Dual-USB ST7789 boards and ELEGOO ILI9341 boards without requiring manual config files.
  * Detects specific board models (`display.modelName()`) and automatically configures color inversion, subpixel order, and touch axis mapping.
* 📦 **100% Self-Contained & Drop-In Compatible**:
  * Provides a complete, standalone `TFT_eSPI` class implementation.
  * Does **not** require installing Bodmer's TFT_eSPI library.
  * Drop-in compatible with existing TFT_eSPI drawing calls (`drawString`, `fillRoundRect`, `fillCircle`, `drawTriangle`, etc.).
* 🔤 **Full Adafruit GFX FreeFonts Support**:
  * Bundles all 48 standard FreeFonts (`FreeSans`, `FreeMono`, `FreeSerif`, `TomThumb`).
  * Drop-in support for `tft.setFreeFont(&FreeSans9pt7b)` and `Free_Fonts.h` shorthand aliases (`FSS9`, `FF17`, etc.).
  * Proportional kerning, baseline alignment, and text measurement across both `tft` and `TFT_eSprite`.
  * Zero-flash bloat: only fonts referenced in your sketch are compiled into flash memory.
* 🧭 **Consistent Landscape Orientation**:
  * Standardized on **Rotation 3** across all library examples, aligning with the physical board design (USB port on the left, MicroSD slot on top).
* 🎨 **Full Off-Screen Sprite Engine (TFT_eSprite)**:
  * **16-bit color** (RGB565 full depth)
  * **8-bit color** (RGB332 low memory)
  * **4-bit color** (16-color indexed palette: run full 240x240 analog gauges in only ~28 KB RAM!)
  * Supports transparent color keying and push-to-display / push-to-sprite.
* 🚀 **Zero-Boilerplate LVGL v8.x & v9.x Support**:
  * Built-in `AMK_CYD_LVGL` helper turns 70+ lines of confusing DMA buffer, display flushing, and touch driver setup into 1 single line (`lvgl.begin(display, touch);`).
  * Features hardware DMA double-buffering in ESP32 internal SRAM for silky-smooth 60 FPS graphics with zero tearing.
  * Bundles a pre-configured `lv_conf.h` so sketches compile immediately out of the box with zero manual configuration.
* 🎨 **Visual GUI Builder Support (SquareLine Studio & EEZ Studio)**:
  * Design screens visually with drag-and-drop widgets (speedometers, switches, charts, sliders) and export directly to your sketch.
  * Launch full interactive UIs in 1 line (`ui_init();`) without writing complex driver glue code.
  * Pre-configured with enabled Montserrat fonts (10pt to 24pt) for seamless out-of-the-box compiling.
* 🔘 **Full TFT_eSPI_Button Drop-In Compatibility**:
  * Complete implementation of Bodmer's `TFT_eSPI_Button` class (`initButton`, `initButtonUL`, `drawButton`, `contains`, `press`, `justPressed`, `justReleased`).
  * Existing TFT_eSPI sketches with on-screen touch keypads and buttons run immediately out of the box with zero code changes!
* 🌈 **Smooth Linear Color Gradients & Alpha Blending**:
  * Built-in `fillRectVGradient()` (vertical) and `fillRectHGradient()` (horizontal) for modern UI cards, headers, and gauge backgrounds.
  * Fast 16-bit RGB565 `alphaBlend(alpha, fg_color, bg_color)` for smooth overlays and transparencies.
  * Fully supported across both the direct screen and `TFT_eSprite` off-screen buffers.
* 💻 **Visual Studio & PlatformIO Ready**:
  * Bundles official `library.json` manifest and safe include guards for clean compiling and IntelliSense in Visual Studio and VS Code PlatformIO.
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
* 🖥️ **ESP32-S3 Parallel RGB & Capacitive Touch Support**:
  * Direct 16-bit parallel RGB interface with ST7262 controller for 800x480 resolution (4.3", 5.0", and 7.0" screens).
  * Built-in CH422G I2C I/O expander driver (controls backlight PWM, LCD reset, touch reset, LCD power, SD CS).
  * Smooth capacitive multi-touch via Goodix GT911 driver.
* 🌐 **Universal Cross-Platform Abstraction**:
  * Cross-platform aliases `AMK_display` and `AMK_touch` automatically resolve to CYD SPI on standard ESP32, and parallel RGB + capacitive touch on ESP32-S3!
  * Generalized classes: `AMK_S3_RGB_display`, `AMK_S3_touch`, and `AMK_S3_Board`.
  * Complete backward compatibility for existing code (`AMK_CYD_display`, `AMK_CYD_touch`, `ESP32S3_Touch_LCD_7_...`).
* 🛑 **Compile-Time Failsafe Protection**:
  * For CYD boards: asserts compile-time warnings if PSRAM is accidentally enabled or flash is set > 4 MB, preventing boot loops and GPIO16/17 pin conflicts.
  * For ESP32-S3 boards: validates PSRAM availability required for large 800x480 RGB framebuffers.

---

## 📋 Supported Hardware Matrix

* **Dual Touch Controller Engine (Resistive XPT2046 + Capacitive GT911 / CST820)**:
  * Runtime auto-probing on I2C (SDA: 33, SCL: 32) for capacitive touch chips (GT911 at 0x5D/0x14, CST820 at 0x15).
  * Automatically falls back to high-speed SPI XPT2046 resistive touch when capacitive is not present.
  * Completely unified API: 	ft.getTouch(&x, &y) and 	ouch.read(x, y) work identically across resistive and capacitive models!
* **Half-Duplex 3-Wire SPI Read on GPIO13 (SDA)**:
  * Automatically probes panel ID registers bidirectionally over GPIO13 for panels lacking SDO wiring (e.g. ESP32-2432S032 ST7789), eliminating blank or distorted screens.
* **Persistent NVS Hardware Overrides (cydrforce)**:
  * Easily save hardware adjustments into ESP32 NVS flash (cydr_saveOverrides) for tricky Chinese clone boards: panel type, color order (RGB/BGR), inversion, MADCTL mirroring, SPI clock (40MHz / 27MHz / 20MHz), backlight pin, touch axes, and RGB LED pins.
* **Bus Contention Protection (	ouchI2cLock)**:
  * Prevents SPI display readbacks from colliding with I2C touch communications when GPIO33 is shared between I2C SDA and unpopulated XPT2046 CS lines.
* **Display Framebuffer Readback Detection (cydr_readback_t)**:
  * Probes and verifies whether pixel memory can be read back over GPIO12 (MISO), GPIO13 (half-duplex SDA), or unavailable (	ft.isReadbackSupported(), 	ft.getReadbackName()).

### 1. ESP32 Cheap Yellow Display (CYD) Series (Resistive & Capacitive Touch)

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
| **ESP32-2432S032** (3.2" CYD) | 3.2" | 240x320 | ST7789 (SDA half-duplex) | Shared HSPI Bus (XPT2046) | GPIO 27 | GPIO 4 |
| **ESP32-2432S032C** (3.2" Capacitive) | 3.2" | 240x320 | ST7789 (SDA half-duplex) | I2C GT911 (SDA 33 / SCL 32) | GPIO 27 | GPIO 4 |
| **ESP32-2432S024C** (2.4" Capacitive) | 2.4" | 240x320 | ILI9341 | I2C CST820 (SDA 33 / SCL 32) | GPIO 27 | GPIO 4 |
| **Guition JC2432W328** (2.8" Capacitive) | 2.8" | 240x320 | ST7789 | I2C CST820 (SDA 33 / SCL 32) | GPIO 27 | GPIO 4 |
| **ESP32-2432S028C** (2.8" Capacitive) | 2.8" | 240x320 | ILI9341 / ST7789 | I2C CST820 (SDA 33 / SCL 32) | GPIO 21 | GPIO 4 |
| **ESP32-3248S035C** (3.5" Capacitive) | 3.5" | 320x480 | ST7796 | I2C GT911 (SDA 33 / SCL 32) | GPIO 27 | GPIO 4 |

### 2. ESP32-S3 Parallel RGB Series (16-bit RGB + GT911 Capacitive Touch)

| Board Model | Screen Size | Resolution | Interface | Driver IC | Touch Controller | Architecture / IO Expander |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|
| **Waveshare ESP32-S3-Touch-LCD-7** | 7.0" | 800x480 | 16-bit Parallel RGB | ST7262 | GT911 (Capacitive) | CH422G (I2C SDA: 8, SCL: 9) |
| **Waveshare ESP32-S3-Touch-LCD-5** | 5.0" | 800x480 | 16-bit Parallel RGB | ST7262 | GT911 (Capacitive) | CH422G (I2C SDA: 8, SCL: 9) |
| **Waveshare ESP32-S3-Touch-LCD-4.3** | 4.3" | 800x480 | 16-bit Parallel RGB | ST7262 | GT911 (Capacitive) | CH422G (I2C SDA: 8, SCL: 9) |
| **Sunton ESP32-8048S070 / S050 / S043** | 4.3" - 7.0" | 800x480 | 16-bit Parallel RGB | ST7262 | GT911 (Capacitive) | Direct GPIO (BL: GPIO 2, Touch: 19/20) * |

> *\* Note on Sunton ESP32-8048S Series:* Sunton S3 boards do not include an onboard CH422G I/O expander and route the backlight directly to GPIO 2 PWM, with GT911 touch on I2C pins 19 & 20. The integrated `AMK_S3_RGB_Board` manager defaults to the Waveshare CH422G architecture.

---

## 🛠️ Recommended Arduino IDE Settings

### For ESP32 CYD Boards:
* **Board**: ESP32 Dev Module
* **Upload Speed**: 921600
* **CPU Frequency**: 240MHz (WiFi/BT)
* **Flash Frequency**: 80MHz
* **Flash Mode**: QIO
* **Flash Size**: 4MB (32Mb) *(Do NOT select 8MB or 16MB)*
* **Partition Scheme**: Default 4MB with spiffs or Huge APP (3MB No OTA/1MB SPIFFS)
* **PSRAM**: Disabled *(Do NOT enable PSRAM; CYD boards do not have external PSRAM, and enabling it locks up GPIO16 & GPIO17)*

### For ESP32-S3 Parallel RGB Boards:
* **Board**: `ESP32S3 Dev Module` or `Waveshare ESP32-S3-Touch-LCD-7`
* **Upload Speed**: 921600
* **USB CDC On Boot**: Enabled *(Required for Serial output)*
* **CPU Frequency**: 240MHz
* **Flash Size**: 16MB (or board specific)
* **Partition Scheme**: 16MB Flash (3MB APP/9.9MB FATFS) or Huge APP
* **PSRAM**: OPI PSRAM (or Enabled) *(Mandatory: 800x480 RGB framebuffers require Octal PSRAM; 120MHz recommended if supported, 80MHz standard)*

---

## 📥 Installation

### Method 1: Official Arduino IDE Library Manager (Recommended)
The fastest and easiest way to install and receive automatic updates:
1. In Arduino IDE, open **Tools** ➔ **Manage Libraries...** (or press `Ctrl + Shift + I`).
2. In the search bar, type: **`AMK_CYD_Universal_TFT`**
3. Click **Install**!

### Method 2: Arduino IDE (ZIP Install)
1. Download this repository as a `.ZIP` file from GitHub (`Code` -> `Download ZIP`).
2. Open **Arduino IDE**.
3. Navigate to **Sketch** -> **Include Library** -> **Add .ZIP Library...**
4. Select the downloaded `AMK_CYD_Universal_TFT.zip`.

### Method 3: Git Clone
Clone directly into your Arduino libraries directory:
```bash
# Windows (PowerShell)
cd C:\Users\PSA\Documents\Arduino\libraries
git clone https://github.com/aungmoekhine/AMK_CYD_Universal_TFT.git

# Linux / macOS
cd ~/Documents/Arduino/libraries
git clone https://github.com/aungmoekhine/AMK_CYD_Universal_TFT.git
```

### Method 4: PlatformIO (`platformio.ini`)
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

### 1. Cross-Platform Display & Touch (Works on CYD and ESP32-S3!)

```cpp
#include <Arduino.h>
#include <AMK_CYD_Universal_TFT.h>

// Universal driver objects: automatically resolves to CYD or ESP32-S3 RGB!
AMK_display display;
AMK_touch   touch;
TFT_eSPI    tft;

void setup() {
  Serial.begin(115200);

  // 1-LINE UNIVERSAL HARDWARE AUTO-SETUP (Rotation 3 = Landscape USB left)
  // Auto-probes display, backlight PWM, LDR light sensor, active-LOW RGB LED, and touch!
  tft.autoSetup(3);

  // Auto-calibrates uncalibrated resistive touch (< 1 ms if already saved; 0 ms if capacitive)
  tft.autoCalibrate();

  // Draw UI
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Hello from AMK Universal TFT!", tft.width() / 2, 40, 2);

  char info[96];
  snprintf(info, sizeof(info), "Panel: %s (%dx%d)", 
           display.panelName(), tft.width(), tft.height());
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(info, tft.width() / 2, 70, 2);
}

void loop() {
  // First-line execution for smooth ambient light auto-dimming
  tft.adjustBrightness();

  uint16_t x, y;
  // Read touch input
  if (tft.getTouch(&x, &y)) {
    tft.fillCircle(x, y, 3, TFT_YELLOW);
  }
  delay(5);
}
```

---

### 2. ESP32-S3 Parallel RGB & GT911 Capacitive Touch (800x480)

```cpp
#include <Arduino.h>
#include <AMK_CYD_Universal_TFT.h>

// Dedicated ESP32-S3 RGB classes
AMK_S3_RGB_display display;
AMK_S3_touch       touch;
TFT_eSPI           tft;

void setup() {
  Serial.begin(115200);

  tft.init();               // Initializes CH422G IO expander & 16-bit RGB bus
  tft.setRotation(0);       // Native 800x480 landscape
  tft.setBacklight(100);    // Backlight controlled via CH422G PWM

  touch.begin(display);     // GT911 capacitive touch over I2C

  tft.fillScreen(TFT_BLACK);
  tft.fillRectVGradient(0, 0, 800, 60, TFT_NAVY, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_TRANSPARENT);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("ESP32-S3 800x480 Parallel RGB Touch Demo", 400, 30, 4);
}

void loop() {
  uint16_t x, y;
  if (tft.getTouch(&x, &y)) {
    tft.fillCircle(x, y, 6, TFT_CYAN);
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
  tft.setRotation(3); // Landscape (USB left, SD top)
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

### 3. Beautiful Typography with FreeFonts

```cpp
#include <Arduino.h>
#include <AMK_CYD_Universal_TFT.h>

AMK_CYD_display display;
AMK_CYD_touch   touch;
TFT_eSPI        tft;

void setup() {
  tft.init();
  tft.setRotation(3); // Landscape (USB left, SD top)
  tft.fillScreen(TFT_BLACK);

  // 1. Direct font pointer:
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("FreeSans 9pt Regular", 20, 30);

  // 2. Shorthand macro from Free_Fonts.h:
  tft.setFreeFont(FSSB12); // FreeSans Bold 12pt
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("FreeSans Bold 12pt", 20, 65);

  // 3. Print / println cursor streams:
  tft.setFreeFont(FM9);    // FreeMono 9pt
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(20, 110);
  tft.print("CPU: 240MHz | Temp: ");
  tft.println(23.5);

  // 4. Reset to classic GLCD 5x7 font:
  tft.setFreeFont(NULL);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Back to classic fixed-pitch font", 20, 150);
}

void loop() {}
```

---

### 4. High-Performance UI with LVGL (Zero-Boilerplate)

`AMK_CYD_Universal_TFT` includes a built-in hardware bridge for **LVGL (Light and Versatile Graphics Library)**, turning 70+ lines of confusing DMA buffer, display flushing, and touch driver setup into **1 single line**:

#### 📥 Easy Setup (Supports LVGL v8.x & v9.x):
1. In Arduino IDE, open **Tools** -> **Manage Libraries...** (or `Ctrl+Shift+I`).
2. Search for **`lvgl`** and click **Install** (*Both LVGL v8.x and v9.x are fully supported!*).
3. In your sketch, `#include <AMK_CYD_LVGL.h>` and call `lvgl.begin(display, touch);`!

> [!NOTE]
> **Pre-Configured `lv_conf.h` Included**: Standard LVGL on Arduino typically requires manually finding and copying `lv_conf.h`. `AMK_CYD_Universal_TFT` bundles an optimized, ready-to-use `lv_conf.h` directly inside its `src/` directory, so your sketch compiles immediately out of the box with zero configuration!

```cpp
#include <Arduino.h>
#include <AMK_CYD_Universal_TFT.h>
#include <AMK_CYD_LVGL.h> // Includes official LVGL automatically!

AMK_CYD_display display;
AMK_CYD_touch   touch;
TFT_eSPI        tft;
AMK_CYD_LVGL    lvgl;

void setup() {
  tft.init();
  tft.setRotation(3); // Landscape (USB left, SD top)
  tft.setBacklight(100);

  touch.begin(display);
  touch.setRotation(3);
  touch.loadCalibration();

  // 1-Line LVGL Initialization:
  // Automatically allocates DMA draw buffers in internal SRAM,
  // configures RGB565 byte-swapping, and maps touch pointers!
  lvgl.begin(display, touch);

  // Now create ANY official LVGL widget (buttons, sliders, gauges, charts):
  lv_obj_t *btn = lv_btn_create(lv_scr_act());
  lv_obj_center(btn);
  lv_obj_t *lbl = lv_label_create(btn);
  lv_label_set_text(lbl, "Hello CYD!");
}

void loop() {
  lvgl.tick(); // Handles LVGL timers, animations, and touch events
  delay(5);
}
```

> [!TIP]
> **Why keep LVGL in Library Manager?**  
> Unlike display hardware drivers, LVGL is a 60+ MB operating system UI framework with hundreds of widgets and animations. Keeping it in the Library Manager prevents your project from becoming bloated, avoids duplicate symbol conflicts with other libraries, and lets you update LVGL directly whenever new upstream bug fixes are released!

---

### 5. Visual Drag-and-Drop GUI Design (SquareLine Studio & EEZ Studio)

Want to design your CYD touch interface visually without hand-coding pixel coordinates? `AMK_CYD_Universal_TFT` provides drop-in support for **[SquareLine Studio](https://squareline.io/)** and **[EEZ Studio](https://www.envox.hr/eez/eez-studio/)**:

1. **Design Visually**: Open SquareLine Studio or EEZ Studio, set resolution to `320x240` (or `480x320`), and drag & drop buttons, sliders, arc dials, and charts onto the screen.
2. **Export UI Files**: Click **Export** ➔ **Export UI Files** and place the generated `ui.h`, `ui.c`, and assets directly into your Arduino sketch folder.
3. **Control Real Hardware**: Connect UI sliders to LCD backlight PWM, switches to onboard LEDs, and dials to sensor telemetry:

```cpp
#include <Arduino.h>
#include <AMK_CYD_Universal_TFT.h>
#include <AMK_CYD_LVGL.h>
#include "ui.h" // Exported from SquareLine or EEZ Studio!

AMK_CYD_display display;
AMK_CYD_touch   touch;
TFT_eSPI        tft;
AMK_CYD_LVGL    lvgl;

// 1. Backlight Slider Event: Controls physical CYD LCD backlight PWM!
void ui_event_slider_brightness(lv_event_t * e) {
    lv_obj_t * slider = (lv_obj_t *)lv_event_get_target(e);
    int32_t val = lv_slider_get_value(slider);
    display.setBacklight((uint8_t)val); // 0-100% PWM brightness
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", (int)val);
    lv_label_set_text(ui_LabelBrightValue, buf);
}

// 2. Onboard RGB LED Switch Event: Toggles the CYD onboard LED!
void ui_event_switch_led(lv_event_t * e) {
    lv_obj_t * sw = (lv_obj_t *)lv_event_get_target(e);
    bool state = lv_obj_has_state(sw, LV_STATE_CHECKED);
    uint8_t pin = display.redLedPin();
    pinMode(pin, OUTPUT);
    digitalWrite(pin, state ? LOW : HIGH); // Active-low (LOW = ON)
}

void setup() {
  tft.init();
  tft.setRotation(3);
  tft.setBacklight(100);

  touch.begin(display);
  touch.setRotation(3);
  touch.loadCalibration();

  // 1-line LVGL initialization & DMA double-buffering
  lvgl.begin(display, touch);

  // Initialize visual GUI!
  ui_init();
}

void loop() {
  lvgl.tick(); // Keeps touch events, timers, and animations running
  delay(5);
}
```
> [!TIP]
> Open **File** ➔ **Examples** ➔ **AMK_CYD_Universal_TFT** ➔ **`VisualGUI_QuickStart`** to try this full interactive smart room dashboard right now!

---

## 🔤 Typography & Font Reference Guide

`AMK_CYD_Universal_TFT` includes two typography engines to give you complete design flexibility:

### 1. Vector FreeFonts (Smooth Anti-Aliased Look)
Bundled out of the box in `src/Fonts/GFXFF/` with **zero flash bloat** (unused fonts are stripped automatically by the compiler linker):

| Family | Available Sizes | Common Shorthand Macros | Approx. Height | Recommended Uses |
|:---|:---:|:---|:---:|:---|
| **FreeSans** | 9pt, 12pt, 18pt, 24pt | `FSS9`, `FSS12`, `FSS18`, `FSS24`<br>`FSSB9`, `FSSB12` (Bold)<br>`FSSO9`, `FSSBO12` (Oblique) | 14px – 34px | UI cards, headers, sleek modern telemetry |
| **FreeMono** | 9pt, 12pt, 18pt, 24pt | `FM9`, `FM12`, `FM18`, `FM24`<br>`FMB9`, `FMB12` (Bold) | 14px – 34px | Fixed-width data, terminal consoles, code readouts |
| **FreeSerif** | 9pt, 12pt, 18pt, 24pt | `FS9`, `FS12`, `FS18`, `FS24`<br>`FSB9`, `FSB12` (Bold)<br>`FSI9`, `FSBI12` (Italic) | 14px – 34px | Elegant classic labels, decorative titles |
| **TomThumb** | Micro (~5px) | `TT1` | ~6px | Ultra-dense telemetry, compact data logs |

### 2. Classic Built-In 5×7 Bitmap Font
Selected whenever FreeFonts are disabled via `tft.setFreeFont(NULL)`:

| Setting | Character Size | Best For |
|:---:|:---:|:---|
| `setTextSize(1)` | **6 × 8 px** | Compact telemetry (e.g. `Heap: 290KB`, small touch coords) |
| `setTextSize(2)` | **12 × 16 px** | Standard tabs, buttons, and alert text |
| `setTextSize(3)` | **18 × 24 px** | Medium dashboard readouts |
| `setTextSize(4..8)`| **24×32 up to 48×64 px** | High-visibility fullscreen counters |

> [!TIP]
> **Pro-Tip: Scaling FreeFonts to Giant Sizes**  
> `setTextSize(scale)` works seamlessly with FreeFonts too! If you need massive numbers (e.g. a 48pt speedometer, huge digital clock digits, or fullscreen temperature gauge), scale a 24pt font by `2`:
> ```cpp
> tft.setFreeFont(&FreeSans24pt7b);
> tft.setTextSize(2); // Multiplies 24pt to 48pt (~68 pixels tall!)
> tft.drawString("98.6°", 40, 60);
> tft.setTextSize(1); // Reset back to standard 1x scale
> ```

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

### Q: CYD Board boot loops with CORRUPT HEAP or PSRAM errors
In your Arduino IDE menu, make sure **Tools** -> **PSRAM** is set to **Disabled**. Classic CYD boards do not have external PSRAM chips.

### Q: ESP32-S3 RGB Display fails to compile or shows black screen
For ESP32-S3 Parallel RGB boards (4.3", 5.0", 7.0"):
1. Set **Tools** -> **PSRAM** to **"OPI PSRAM"** (or "Enabled"). The 800x480 double-framebuffers require PSRAM.
2. Set **Tools** -> **USB CDC On Boot** to **"Enabled"** to view `Serial.print()` output.
3. Select **Board**: `Waveshare ESP32-S3-Touch-LCD-7` or `ESP32S3 Dev Module`.

### Q: Horizontal lines, screen tearing, or flickering glitches on 800x480 RGB panels
On large 800x480 dot-clock RGB panels, the ESP32-S3 must continuously stream ~30-40 MB/s from external PSRAM to the display. If the PSRAM bus is saturated by Wi-Fi, CPU, or flash writes, the display DMA buffer can briefly starve, causing visual flicker or drift.
* Follow the [ESP32-S3 RGB Performance & Stability Guidelines](#-esp32-s3-rgb-800x480-performance--stability-guidelines) below to completely eliminate glitches.

---

## 🏎️ ESP32-S3 RGB (800x480) Performance & Stability Guidelines

Driving a large 800x480 dot-clock RGB panel (Waveshare & Sunton 4.3", 5.0", 7.0") requires moving **~768 KB per frame** (over **30 Megabytes per second** at 40 FPS) over the ESP32-S3's external PSRAM bus. Because the ESP32-S3 lacks a hardware priority arbiter between the CPU, Wi-Fi, and display DMA, heavy concurrent memory access can occasionally cause DMA underflow (manifesting as horizontal tearing, shifting, or flickering bands).

To ensure smooth, glitch-free graphics, follow these recommended practices:

### 1. Arduino IDE / PlatformIO PSRAM Settings
* **PSRAM Mode**: Always select **"OPI PSRAM"** (Octal SPI). Never use Quad SPI for 800x480 panels.
* **PSRAM Frequency**: If your board has an ESP32-S3 N8R8 or N16R8 module, set PSRAM Speed to **120 MHz** (or 80 MHz fallback). A 120 MHz Octal bus provides ~120 MB/s bandwidth, giving the display plenty of breathing room.
* **CPU Frequency**: Keep set to **240 MHz**.

### 2. Protect Against SPI Flash / LittleFS Stalls
During SPI Flash write operations (`LittleFS`, `SPIFFS`, or EEPROM/NVS commits), the ESP32 CPU cache is temporarily disabled, stalling PSRAM reads and causing visible scanline tearing.
* **Solution**: Wrap your file/flash operations using the built-in flash message helper:
  ```cpp
  // Displays a lightweight SRAM-based message or blank screen while flash writes:
  display.beginFlashMessage("SAVING DATA...");
  // ... perform your LittleFS / Flash write ...
  display.endFlashMessage();
  ```

### 3. Place Dynamic Sprites & Buffers in Internal SRAM
* **Why**: The bounce-buffer copy task runs at high priority during display refresh. Reading sprite data out of external PSRAM during the refresh cycle adds bus contention and cache misses.
* **Solution**: Keep small dynamic sprites, icons, and gauge overlays in internal SRAM (`MALLOC_CAP_INTERNAL`):
  ```cpp
  uint16_t* spriteBuf = (uint16_t*)heap_caps_malloc(width * height * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  ```

### 4. Isolate Display and Wi-Fi Cores
* By default, `AMK_CYD_Universal_TFT` pins the RGB bounce-fill task to **Core 1** (`LCD7_BOUNCE_FILL_OTHER_CORE 0`).
* Keep heavy Wi-Fi / Bluetooth networking tasks pinned to **Core 0** (the FreeRTOS PRO CPU default). This ensures that heavy network packet processing never preempts the time-critical display refresh task.

### 5. Tune Pixel Clock (`pclk_hz`) for Extreme Wi-Fi / TLS Loads
* If your sketch runs heavy continuous Wi-Fi encryption (e.g., HiveMQ TLS / HTTPS streaming) and you notice occasional scanline jitter:
  * Lowering the panel pixel clock from 16-18 MHz down to **12 MHz - 14 MHz** reduces PSRAM bandwidth demand by **25% - 30%**.
  * The screen will still update at a fluid ~30-35 FPS, completely eliminating DMA underflow under heavy network loads.

---

## 📂 Included Examples

| Example | Description |
|:---|:---|
| **`Universal_AutoConfig`** | **Master Auto-Configuration Template [NEW in v1.6.0]**. Demonstrates 1-line `tft.autoSetup(3)`, adaptive multi-point calibration (`tft.autoCalibrate()`), responsive hardware telemetry cards, 7-color verification bar, and live touch tracking with RGB LED sync. |
| **`CYD_Universal_AutoConfig`** | Reference implementation of the hardware auto-detection engine, illustrating explicit step-by-step probing mechanics, LDR auto-dimming filter math, and Active-LOW LED control. |
| **`QuickStart`** | Minimal boilerplate template showcasing hardware auto-detection, status logging, and a simple touch dot drawing loop. |
| **`BoardDetect`** | Standalone hardware detection and diagnostic tool. Probes board model, panel type, resolution, diagonal size, touch architecture, readback channel, and RGB LED pins. |
| **`BasicTouchDemo`** | Full touch diagnostic canvas showing real-time `(X, Y)` touch coordinates, crosshair tracking, pressure values, and screen boundaries. |
| **`ButtonDemo`** | Drop-in Bodmer `TFT_eSPI_Button` showcase. Features interactive touch buttons with press/release states, inverted click feedback, and hardware control (RGB LED & backlight toggle). |
| **`GradientDemo`** | Showcase of linear color gradients (`fillRectVGradient`, `fillRectHGradient`) and alpha blending (`alphaBlend`). Features a sleek multi-card dashboard, frosted-glass HUD strip, theme palette cycling via touch, and onboard RGB LED sync. |
| **`VisualGUI_QuickStart`** | Full drag-and-drop visual GUI template compatible with SquareLine Studio and EEZ Studio. Features an interactive temperature arc dial, live backlight PWM slider, onboard RGB LED toggle switch, and button callbacks. |
| **`LVGL_QuickStart`** | 1-line LVGL initialization boilerplate showcasing automated DMA buffer allocation, display flushing, and touch pointer mapping with interactive buttons and sliders. |
| **`FreeFontDemo`** | Comprehensive typography showcase featuring 5 interactive pages: font families comparison, proportional point sizes, giant vector scaling (`setTextSize(2)`), accurate text datum alignment anchors, and smooth dynamic off-screen sprite telemetry. |
| **`TouchCalibration`** | Interactive multi-stage calibration tool. Features 4-point crosshair calibration, pressure thresholding, jump filters, a live test drawing canvas, and persistent storage in NVS Flash. |
| **`SpriteDemo`** | High-performance 5-tab UI dashboard: <br>• **`METER`**: Smooth 124x124 analog power gauge sprite + backlight brightness drag slider.<br>• **`WAVE`**: Real-time 270x110 oscilloscope waveform buffer with speed and pause controls.<br>• **`RGB/LDR`**: Onboard active-low RGB LED color tester and LDR photoresistor ambient light level bar.<br>• **`DRAW`**: Interactive touch drawing canvas with 7-color square palette selection and a Clear button.<br>• **`LOGO`**: AMK Studio splash screen with auto-detected board model, panel name, resolution, and RAM telemetry. |
| **`ESP32_S3_RGB_Touch_Demo`** | **Universal ESP32-S3 Parallel RGB & Capacitive Multi-Touch showcase (800x480)**. Features high-speed direct framebuffer rendering, 3 interactive `TFT_eSPI_Button` controls, live touch point tracking with coordinates, and counter. |

---

## 📝 Release History & Changelog

### v1.6.3 (Universal Hardware Expansion & Stability Release)
* 🛡️ **Rock-Solid Synchronized Rendering Pipeline**:
  * Enforces strict pre-blit DMA synchronization (display.waitFlush()) and rock-solid scanline buffering across _fill, pushImage, and pushSprite.
  * Completely eliminates DMA memory race conditions, vertical display displacement, and card edge ghosting/duplication on classic CYD resistive & capacitive panels.
* 👆 **Resistive Touch Noise & Ghost-Click Suppression**:
  * Defaulted 7-sample moving median filtering and minimum pressure thresholds in 	ft.autoSetup() (	ouch.setFilters(200, 50)).
  * Eliminates phantom ghost touches, high-impedance ADC jitter, and false release events on XPT2046 resistive panels out of the box.
* 📚 **Universal Hardware Database & Profiles (AMK_Hardware_Profiles.h)**:
  * Comprehensive built-in hardware database extracted and verified against production schematics and the OWLOS flight radar core.
  * Added official model support and pin configurations for:
    * **Elecrow CrowPanel Advance 2.8"**: ESP32-S3 HMI, ST7789 IPS, FT6336U capacitive touch (I2C SDA 15, SCL 16), PWM backlight on GPIO 38.
    * **LCDWIKI ES3C28P 2.8"**: ESP32-S3 N16R8, ILI9341V IPS, FT6336G capacitive touch, onboard WS2812B RGB LED on GPIO 42, PWM backlight on GPIO 45.
    * **NM-CYD-C5 2.8"**: ESP32-C5 CYD form factor, ST7789, XPT2046 resistive touch (CS 1, IRQ 3), WS2812C RGB LED on GPIO 27, backlight on GPIO 25.
    * **Sunton ESP32-8048S050 / Guition JC8048W550C 5.0" RGB**: Direct GPIO 2 backlight fallback, GT911 touch (I2C SDA 19, SCL 20).
    * **Elecrow CrowPanel Advance 5.0" RGB**: ST7262 Parallel RGB, GT911 touch (I2C SDA 15, SCL 16).
  * Added sketch-accessible helper APIs: 	ft.getBoardProfile(), 	ft.getAuxLedPin(), and 	ft.getAudioAmpPin().
  * Enhanced 	ft.printHardwareReport() with MCU platform info, hardware notes, and onboard peripheral assignments.
* 🖥️ **S3 Parallel RGB & CYD Full API Parity**:
  * Added 
ativeWidth(), 
ativeHeight(), and flexible 6-parameter lushRegion() overload to chipguy_S3_RGB_display.h, guaranteeing 100% binary-compatible sketch code across both Classic CYD and S3 RGB boards.
* 🔒 **Strict Preservation of Global Hardware Standards (RULE[user_global])**:
  * Color order tables, MADCTL settings, and RGB/BGR definitions remain completely frozen with zero regression risk.
  * Backlight dynamic auto-detection (display.backlightPin()) and LDR 10-read boot priming remain strictly maintained.

### v1.6.2 (PlatformIO & Multi-Core Cross-Compatibility Patch)
* 🛠️ **Universal PlatformIO & VSCode Support**:
  * Resolves all compilation and linking failures when building under VSCode and PlatformIO across standard and bleeding-edge frameworks.
* ⚡ **Cross-Version ESP-IDF & ESP32 Arduino Core Compatibility**:
  * **ESP-IDF v5.5 / v5.3 / v5.1 / v4.4 Guarded SPI Overrides**: Added preprocessor guards around `spi_transaction_t::override_freq_hz`, ensuring seamless builds on IDF 5.3, 5.1, and 4.4 toolchains where this struct member is not defined.
  * **Arduino Core 2.x & 3.x Dual-Mode `analogWriteResolution`**: Added version-aware switching between Core 3.x dual-parameter `analogWriteResolution(pin, bits)` and Core 2.x single-parameter `analogWriteResolution(bits)`.
  * **Dynamic Peripheral Manager Guard**: Wrapped `<esp32-hal-periman.h>` and `perimanClearPinBus(4)` with `#if __has_include(<esp32-hal-periman.h>)`, providing clean fallback for Arduino ESP32 Core 2.0.x toolchains.
  * **C++11 Constexpr Compliance**: Guarded Arduino IDE-specific FQBN validator `cydr_fqbnHasOption()` to avoid C++11 loop syntax restrictions in standard PlatformIO GCC 8 toolchains.

### v1.6.1 (Maintenance & Usability Patch)
* 🐛 **Fixed CYD Backend Minimal Sketch Linker Error**:
  * Added weak symbol fallback (`__attribute__((weak))`) for `chipguy_CYD_resistive_touch touch` in the CYD backend (`AMK_CYD_Universal_TFT.cpp`).
  * Allows end users to write clean minimal sketches with only `TFT_eSPI tft;` without requiring explicit global `AMK_touch touch;` declarations.
* 📱 **Official Model Identification for ESP32-2432S028C (2.8" Capacitive CYD)**:
  * Added `CYDR_MODEL_2432S028C` to `cydr_model_t` enum and runtime detection.
  * 2.8" capacitive CYD boards (CST820 / CST816 on I2C SDA: 33, SCL: 32 with GPIO 21 backlight) are now officially identified as `"ESP32-2432S028C 2.8\""` in the serial hardware audit report instead of `CYDR_MODEL_UNKNOWN`.
* 📋 **ESP32-S3 Parallel RGB Architecture Clarifications**:
  * Documented that the integrated 800x480 parallel RGB driver is tailored for the **Waveshare ESP32-S3-Touch-LCD (CH422G I2C IO Expander)** architecture, detailing the hardware pin differences with Sunton ESP32-8048Sxxx boards (which use direct GPIO 2 PWM backlight and direct touch GPIOs 19/20).

### v1.6.0 (Major Milestone Release)
* 🪄 **1-Line Universal Hardware Auto-Setup Engine (`tft.autoSetup()`)**:
  * Eliminates 30–50 lines of duplicate hardware initialization boilerplate across all sketches!
  * Single call (`tft.autoSetup(rotation)`) completely initializes the board at runtime:
    * **Display Panel & Bus**: Calls `init()`, sets rotation, enforces `setSwapBytes(true)`, and verifies MADCTL color mapping.
    * **Backlight Pin & 8-Bit PWM**: Dynamically detects GPIO 21 vs GPIO 27 with zero hardcoded pins and zero fallback pins.
    * **LDR Ambient Light Sensor**: Detects GPIO 34 role, configures input, and primes the 10-read moving average filter before the panel turns on.
    * **Active-LOW RGB LED**: Detects Red (GPIO 4 vs 22), Green, and Blue pins, initializes them OFF (`HIGH`), and runs an instant R-G-B boot test sequence.
    * **Touch Subsystem**: Probes Resistive (XPT2046) vs Capacitive (GT911/CST820), syncs rotation, and auto-loads persistent NVS calibration.
    * **Serial Diagnostic Audit**: Automatically prints a full hardware specification report at 115200 baud (`printHardwareReport()`).
* 🎯 **Adaptive Multi-Point Statistical Touch Calibration Engine (`tft.autoCalibrate()`)**:
  * Eliminates the need to flash a separate calibration sketch! Built directly into the library.
  * **Zero Boot Overhead (< 1 ms)** if calibration is already stored in NVS flash.
  * **Zero Boot Overhead (0 ms)** if capacitive touch (CST820 / GT911) is detected.
  * **Adaptive 12-to-30 Point Grid ($4 \times 3$ to $6 \times 5$)**: Runs multi-sample ADC averaging with raw jitter rejection (`JITTER_RAW = 130`, `MIN_SAMPLES = 8`) to reject shaky hand movements.
  * Full-screen crosshair guides with sub-pixel visual accuracy.
  * Independent 2D Linear Regression (`linfit`) mapped to native panel dimensions with hardware mirror compensation (`mirrorsX()`, `mirrorsY()`).
  * Strict Statistical RMS and Max Error verification (`ACCEPT_RMS <= 6.0 px`, `ACCEPT_MAX <= 16.0 px`).
  * Automatically commits calibrated boundaries and filter parameters to ESP32 NVS flash (`touch.saveCalibration()` + `touch.saveFilters()`) so subsequent boots load instantly.
  * **Hardware BOOT Pin Override**: Holding the CYD `BOOT` button (GPIO 0) at startup forces re-calibration on demand.
  * Upgraded legacy `calibrateTouch()` API to route directly through the new high-precision multi-point engine.
  * Added `hasTouchCalibration()` inspection helper.
* 💡 **Built-In Ambient Light Auto-Dimming (`tft.adjustBrightness()`)**:
  * Single call as the first line of `void loop()` guarantees smooth, flicker-free auto-dimming with an integrated 10-sample moving average buffer and 4-unit hysteresis deadband.
  * Fully non-blocking and immune to network delays or client handshakes.
* 🚥 **Built-In Universal RGB LED Helpers (Active-LOW Polarity)**:
  * Drive onboard RGB LEDs effortlessly without worrying about Active-LOW polarity or pin variants: `tft.setRgbLed(r, g, b)`, `tft.turnLedOff()`, `tft.flashLed(r, g, b, ms)`, `tft.flashLedSequence()`.
* 🔍 **Comprehensive Hardware Query Getters**:
  * Added direct hardware query methods: `redLedPin()`, `greenLedPin()`, `blueLedPin()`, `hasLdrSensor()`, `isBattery()`, `readBatteryVoltage()`, `modelName()`, `panelName()`, `diagonalTenths()`, `nativeWidth()`, `nativeHeight()`.
* 📦 **New Master Example Sketch (`Universal_AutoConfig`)**:
  * Added `Universal_AutoConfig.ino` master template showcasing pure 1-line auto-probing, responsive UI card telemetry, 7-color verification swatches, interactive touch drawing canvas, and real-time RGB LED synchronization.
* 🔄 **Standardized All Example Sketches & Projects**:
  * Updated all bundled example sketches, `Mini_Weather_Station_2.8_CYD`, and `CYD_FlipClock_Universal_Modified` to adhere strictly to the new v1.6.0 hardware standards.

### v1.5.1
* **Capacitive CYD & Dual Touch Controller Architecture**:
  * Added native auto-probing and support for 5 new boards:
    * **ESP32-2432S032**: 3.2" CYD ST7789 with half-duplex SDA SPI readback.
    * **ESP32-2432S032C**: 3.2" CYD with GT911 capacitive touch on I2C (SDA: 33, SCL: 32).
    * **ESP32-2432S024C**: 2.4" CYD with CST820 capacitive touch on I2C + ILI9341.
    * **Guition JC2432W328**: 2.8" CYD with CST820 capacitive touch on I2C + ST7789.
    * **ESP32-3248S035C**: 3.5" CYD with GT911 capacitive touch on I2C + ST7796.
  * **Dual Touch Controller Engine**: Runtime auto-detection between XPT2046 resistive touch and GT911 / CST820 capacitive touch chips. Seamlessly unifies coordinates and rotations with zero code changes.
* **Half-Duplex 3-Wire SPI Read on GPIO13 (SDA)**:
  * Solved panel identification failure on CYD boards without SDO (GPIO12) lines by reading registers bidirectionally over GPIO13.
* **Persistent NVS Hardware Overrides Engine (cydrforce)**:
  * Store board-level corrections (panel type, color order, inversion, mirror bits, SPI clock, backlight pin, touch axes, and RGB LED pins) in ESP32 NVS flash, permanently correcting clone or uncatalogued boards via calibration.
* **Bus Contention Protection (cydr_touchI2cLock / cydr_touchI2cUnlock)**:
  * Prevents SPI display readbacks from colliding with I2C touch communications when GPIO33 is shared between I2C SDA and unpopulated XPT2046 CS lines.
* **Display Framebuffer Readback Detection (cydr_readback_t)**:
  * Added 	ft.isReadbackSupported() and 	ft.getReadbackName() to inspect hardware framebuffer readback capability (MISO, SDA, or None).
* **Enhanced Touch Calibration & New Example**:
  * Updated TouchCalibration with automatic capacitive touch detection (bypasses resistive 4-point calibration and jumps directly to interactive touch verification).
  * Added new **BoardDetect** standalone diagnostic example tool.

### v1.5.0
* **ESP32-S3 Parallel RGB & GT911 Capacitive Touch Architecture**:
  * Added complete native support for ESP32-S3 parallel RGB LCD displays (Waveshare & Sunton 4.3", 5.0", and 7.0" 800x480 ST7262).
  * Direct 16-bit parallel RGB DMA engine with continuous auto-refresh.
  * Direct framebuffer drawing optimizations for `fillRect`, `drawPixel`, `pushImage`, and `fillScreen`.
  * Expanded internal line buffers to 800px to fully support wide 800x480 resolution without buffer overflow.
* **Universal Cross-Platform Abstraction**:
  * Cross-platform class aliases `AMK_display` and `AMK_touch` automatically resolve to CYD resistive SPI drivers on ESP32, and parallel RGB + capacitive touch on ESP32-S3.
  * Generalized driver classes: `chipguy_S3_RGB_display`, `chipguy_S3_capacitive_touch`, `AMK_S3_RGB_display`, `AMK_S3_touch`, and `AMK_S3_Board`.
* **Integrated CH422G I2C Expander Controller (`AMK_S3_RGB_Board.h`)**:
  * Self-contained driver for the onboard CH422G I/O expander controlling LCD reset, LCD power, touch reset, backlight, and SD CS with FreeRTOS mutex thread-safety.
* **Goodix GT911 Capacitive Multi-Touch Driver (`chipguy_S3_capacitive_touch`)**:
  * Built-in multi-touch driver supporting unified `read(x, y)` coordinate detection and native orientation synchronization.
* **Zero Overhead & 100% Backward Compatibility for Classic CYD**:
  * All S3 code is strictly guarded with `#if defined(CONFIG_IDF_TARGET_ESP32S3)`. Classic CYD compiles produce empty translation units with **0 extra bytes of RAM or Flash overhead**.
* **New Example Sketch**:
  * Added [`ESP32_S3_RGB_Touch_Demo`] showing 800x480 interactive UI cards, gradients, 3 `TFT_eSPI_Button` elements, and real-time capacitive touch tracking.

### v1.4.0
* **Linear Color Gradients & Advanced Effects**:
  * Added `fillRectVGradient(x, y, w, h, color1, color2)`: Fast vertical linear color gradient rendering.
  * Added `fillRectHGradient(x, y, w, h, color1, color2)`: Fast horizontal linear color gradient rendering.
  * Gradients are fully supported on both the direct screen and inside `TFT_eSprite` off-screen buffers.
  * Added `alphaBlend(alpha, fg_color, bg_color)`: High-speed 16-bit RGB565 alpha blending.
  * Added `color16to24(c)`: 16-bit to 24-bit RGB expansion helper.
* **`TFT_eSPI_Button` Datum & Padding State Preservation**:
  * Enhanced `drawButton()` to record and restore `getTextDatum()` and `getTextPadding()` upon completion, preventing interference with surrounding sketch text alignment.
* **Display State Getters**:
  * Added `getTextDatum()` and `getTextSize()` getters to `TFT_eSPI`.
* **New `GradientDemo` Example Sketch**:
  * Features a modern 3-card telemetry UI with linear horizontal & vertical gradients, frosted-glass alpha-blended HUD, interactive theme palette cycling via touch, and onboard RGB LED synchronization.
* **Hardware-Calibrated `ButtonDemo`**:
  * Calibrated header and subtitle font sizes for physical 2.8" and 2.4" CYD screens, eliminating edge overflow.
  * Re-architected status card drawing with full-box clearing to guarantee zero ghosting or residual characters.

### v1.3.1
* **Full `TFT_eSPI_Button` Drop-In Compatibility**:
  * Added complete standalone implementation of Bodmer's `TFT_eSPI_Button` class (`initButton`, `initButtonUL`, `drawButton`, `contains`, `press`, `isPressed`, `justPressed`, `justReleased`, `setLabelDatum`).
  * Existing sketches and community apps that use on-screen touch keypads and buttons now work seamlessly with zero modifications!
  * Added new **`ButtonDemo`** example sketch.
* **Visual Studio & PlatformIO Support**:
  * Added official `library.json` manifest for native PlatformIO dependency management and include path resolution.
  * Added safe `#if __has_include(<lvgl.h>)` guards in `AMK_CYD_LVGL.h` to prevent compiler errors and red squiggles in Visual Studio / PlatformIO projects when LVGL is not installed.

### v1.3.0
* **Visual Drag-and-Drop GUI Support (SquareLine Studio & EEZ Studio)**:
  * Full out-of-the-box compatibility with exported C/C++ code from SquareLine Studio and EEZ Studio.
  * Added new **`VisualGUI_QuickStart`** example featuring a responsive smart thermostat and hardware control dashboard (real-time backlight PWM slider, RGB LED toggle, temperature arc dial).
  * **Expanded Montserrat Typography in `lv_conf.h`**: Enabled Montserrat 10, 12, 14, 16, 18, 20, 22, and 24 to guarantee zero compile errors when importing complex multi-font visual designs.
  * Added visual GUI integration guide and project setup instructions.

### v1.2.1
* **Hardware DMA Synchronization & Tear-Free Rendering**:
  * Implemented true hardware DMA double-buffering in internal SRAM (`_draw_buf_mem1` and `_draw_buf_mem2`).
  * Added explicit `_display->waitFlush()` hardware synchronization before `lv_display_flush_ready()`, completely eliminating horizontal white tearing bars and color inversion artifacts during high-speed rendering.
  * Added explicit C++ pointer cast in `LVGL_QuickStart` event callback for strict compiler compatibility.

### v1.2.0
* **Zero-Boilerplate LVGL Integration Helper (`AMK_CYD_LVGL.h`)**:
  * Added `AMK_CYD_LVGL` class supporting **both LVGL v8.x and v9.x** with runtime version detection (`LVGL_VERSION_MAJOR`).
  * Automatically handles DMA draw buffer allocation in ESP32 internal SRAM, RGB565 byte-swapping (`setSwapBytes`), display flushing (`flushRegion`), and touch pointer mapping (`touch.read`) in 1 line: `lvgl.begin(display, touch);`.
  * **Bundled `lv_conf.h`**: Ships with a pre-configured, optimized `lv_conf.h` so users don't have to manually create or copy configuration files.
  * Preserves 100% backward compatibility with zero RAM/flash overhead for non-LVGL users.
  * Added `LVGL_QuickStart` example.

### v1.1.1 (First Official Public Release)
* **Universal Hardware Probing & Zero Configuration**:
  * Out-of-the-box runtime auto-detection for the entire ESP32 CYD family (2.4", 2.8", 3.2", 3.5", 4.0").
  * Automatic controller probing for ST7789, ILI9341, and ST7796 displays via SPI register reads.
  * Smart 2.8" Silent-MISO probing on LDR (GPIO 34) to automatically differentiate between Dual-USB ST7789 and ELEGOO ILI9341 boards.
  * Automatic backlight pin mapping (GPIO 21 on 2.8", GPIO 27 on other variants) and PWM brightness control (`setBacklight()`).
  * Automatic RGB LED pin detection (GPIO 4 vs GPIO 22).
  * Seamless subpixel color order handling (BGR vs RGB) across all panels with zero manual tweaking.
* **Adafruit GFX / FreeFonts Typography Engine**:
  * Bundled all 48 standard FreeFonts (`FreeSans`, `FreeSerif`, `FreeMono`, `TomThumb`) in `src/Fonts/GFXFF/`.
  * Shorthand macros via `Free_Fonts.h` (`FSS9`, `FSSB12`, `FM18`, `FSB24`, `TT1`, etc.) and direct pointer support (`&FreeSans9pt7b`).
  * Vector font scaling via `setTextSize(scale)` (e.g. 2x scale on 24pt produces ~68px tall hero telemetry numbers).
  * Proportional kerning, accurate baseline and text measurement (`textWidth()`, `fontHeight()`), and text datum anchors (`setTextDatum`).
  * Instant fallback to classic 5x7 fixed-pitch font via `setFreeFont(NULL)`.
  * Seamless support across both the physical display (`tft`) and RAM buffers (`TFT_eSprite`).
* **Full Off-Screen Sprite Engine (`TFT_eSprite`)**:
  * 16-bit RGB565 full color, 8-bit RGB332 low RAM, and 4-bit 16-color indexed palette modes.
  * Eliminates screen flickering with instant DMA/SPI buffer pushes.
* **Universal XPT2046 Resistive Touch Engine**:
  * Automatically handles both dedicated bit-bang pins (2.8") and shared HSPI bus arbitration (2.4", 3.2", 3.5", 4.0").
  * Multi-stage calibration tool (`TouchCalibration`) with model-isolated NVS flash persistence and adjustable noise/jump filters.
* **Standardized Landscape Orientation**:
  * Standardized on **Rotation 3** across all library examples (USB port on left, MicroSD slot on top).
* **Complete Example Suite**:
  * `QuickStart`: Minimal hardware test boilerplate.
  * `BasicTouchDemo`: Real-time crosshair coordinates and boundary diagnostics.
  * `TouchCalibration`: 4-corner interactive calibration and NVS flash storage.
  * `SpriteDemo`: High-performance 5-tab dashboard (`METER`, `WAVE`, `RGB/LDR`, `DRAW`, `LOGO`).
  * `FreeFontDemo`: 5-page interactive typography showcase (families, sizes, scaling, datums, smooth sprite telemetry).

---

## 👤 Author & Acknowledgments

* **Author & Maintainer**: Aung Moe Khine ([@aungmoekhine](https://github.com/aungmoekhine) • [aungmoekhine@gmail.com](mailto:aungmoekhine@gmail.com))
* **Upstream Inspiration & Open-Source Attribution**:
  * **[chipguyhere](https://github.com/chipguyhere)**: Low-level CYD hardware probing routines, backlight detection, touch SPI arbitration, and ESP32-S3 RGB display driver architecture (MIT License).
  * **[TAMC](https://github.com/tamctec/gt911-arduino)**: GT911 capacitive touch driver foundation (Apache-2.0 License).
  * **[Bodmer (TFT_eSPI)](https://github.com/Bodmer/TFT_eSPI)**: Graphics primitive algorithms, color conversions, and off-screen sprite architecture (MIT/FreeBSD License).
  * **[Adafruit Industries](https://github.com/adafruit/Adafruit-GFX-Library)**: Adafruit GFX typography system and FreeFonts (BSD License).
* **License**: MIT License with third-party attribution notices. Free for hobbyist, educational, and commercial projects.
