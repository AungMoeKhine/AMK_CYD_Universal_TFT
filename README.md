# AMK_CYD_Universal_TFT

[![Arduino Compatible](https://img.shields.io/badge/Arduino-Compatible-00979C.svg?style=flat-square&logo=arduino)](https://www.arduino.cc/)
[![Release](https://img.shields.io/badge/Release-v1.3.1-blue.svg?style=flat-square)](https://github.com/aungmoekhine/AMK_CYD_Universal_TFT/releases)
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
  tft.setRotation(3);       // Landscape (Rotation 3: USB left, SD card top)
  tft.setBacklight(100);    // 0 to 100% PWM brightness

  // 2. Initialize touch and sync rotation
  touch.begin(display);
  touch.setRotation(3);

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

### Q: Board boot loops with CORRUPT HEAP or PSRAM errors
In your Arduino IDE menu, make sure **Tools** -> **PSRAM** is set to **Disabled**. CYD boards do not have external PSRAM chips.

---

## 📂 Included Examples

| Example | Description |
|:---|:---|
| **`QuickStart`** | Minimal boilerplate template showcasing hardware auto-detection, status logging, and a simple touch dot drawing loop. |
| **`BasicTouchDemo`** | Full touch diagnostic canvas showing real-time `(X, Y)` touch coordinates, crosshair tracking, pressure values, and screen boundaries. |
| **`ButtonDemo`** | Drop-in Bodmer `TFT_eSPI_Button` showcase. Features interactive touch buttons with press/release states, inverted click feedback, and hardware control (RGB LED & backlight toggle). |
| **`VisualGUI_QuickStart`** | Full drag-and-drop visual GUI template compatible with SquareLine Studio and EEZ Studio. Features an interactive temperature arc dial, live backlight PWM slider, onboard RGB LED toggle switch, and button callbacks. |
| **`LVGL_QuickStart`** | 1-line LVGL initialization boilerplate showcasing automated DMA buffer allocation, display flushing, and touch pointer mapping with interactive buttons and sliders. |
| **`FreeFontDemo`** | Comprehensive typography showcase featuring 5 interactive pages: font families comparison, proportional point sizes, giant vector scaling (`setTextSize(2)`), accurate text datum alignment anchors, and smooth dynamic off-screen sprite telemetry. |
| **`TouchCalibration`** | Interactive multi-stage calibration tool. Features 4-point crosshair calibration, pressure thresholding, jump filters, a live test drawing canvas, and persistent storage in NVS Flash. |
| **`SpriteDemo`** | High-performance 5-tab UI dashboard: <br>• **`METER`**: Smooth 124x124 analog power gauge sprite + backlight brightness drag slider.<br>• **`WAVE`**: Real-time 270x110 oscilloscope waveform buffer with speed and pause controls.<br>• **`RGB/LDR`**: Onboard active-low RGB LED color tester and LDR photoresistor ambient light level bar.<br>• **`DRAW`**: Interactive touch drawing canvas with 7-color square palette selection and a Clear button.<br>• **`LOGO`**: AMK Studio splash screen with auto-detected board model, panel name, resolution, and RAM telemetry. |

---

## 📝 Release History & Changelog

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
  * **[chipguyhere](https://github.com/chipguyhere)**: Low-level CYD hardware probing routines, backlight detection, and touch SPI arbitration (MIT License).
  * **[Bodmer (TFT_eSPI)](https://github.com/Bodmer/TFT_eSPI)**: Graphics primitive algorithms, color conversions, and off-screen sprite architecture (MIT/FreeBSD License).
  * **[Adafruit Industries](https://github.com/adafruit/Adafruit-GFX-Library)**: Adafruit GFX typography system and FreeFonts (BSD License).
* **License**: MIT License with third-party attribution notices. Free for hobbyist, educational, and commercial projects.
