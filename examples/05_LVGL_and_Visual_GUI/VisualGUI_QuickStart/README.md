# Visual GUI (SquareLine & EEZ Studio) Integration Guide

This example demonstrates how to run visual UIs designed in **SquareLine Studio** or **EEZ Studio** directly on any ESP32 Cheap Yellow Display (CYD) board using **AMK_CYD_Universal_TFT**.

---

## 🎨 How to Export Your Own UI from SquareLine Studio

### Step 1: Create Your Project in SquareLine Studio
1. Open **SquareLine Studio**.
2. Click **Create** ➔ Select **Arduino** (or **Desktop/Eclipse** with LVGL v8 / v9).
3. Set your project settings:
   * **Resolution**: `320` width × `240` height *(or `480` × `320` for 3.5" / 4.0" CYD models)*.
   * **Color Depth**: `16-bit RGB565`
4. Design your screens, buttons, sliders, gauges, and animations visually.

### Step 2: Export UI Files
1. In SquareLine Studio top menu, click **Export** ➔ **Export UI Files**.
2. Choose a destination folder on your computer.
3. SquareLine will generate:
   * `ui.h` and `ui.c`
   * `ui_events.h` and `ui_events.c`
   * Font and Image files (if used in your design).

### Step 3: Put Files in Your Arduino Sketch
1. Copy the exported `.c`, `.h`, and image folders into your Arduino sketch directory (alongside your `.ino` file).
2. In your `.ino` sketch:
   ```cpp
   #include <AMK_CYD_Universal_TFT.h>
   #include <AMK_CYD_LVGL.h>
   #include "ui.h" // Your exported UI header!

   AMK_CYD_display display;
   AMK_CYD_touch   touch;
   TFT_eSPI        tft;
   AMK_CYD_LVGL    lvgl;

   void setup() {
     tft.init();
     tft.setRotation(3);
     touch.begin(display);
     touch.setRotation(3);
     touch.loadCalibration();

     // 1-line LVGL & DMA double-buffer setup
     lvgl.begin(display, touch);

     // Launch your SquareLine UI!
     ui_init();
   }

   void loop() {
     lvgl.tick();
     delay(5);
   }
   ```
3. Upload to your CYD board!

---

## 💡 Hardware Control Tips:
* **Backlight Control**: Call `display.setBacklight(value)` (0-100) inside slider callbacks to control the screen brightness directly from your UI.
* **Onboard RGB LED**: Query `display.redLedPin()`, `display.greenLedPin()`, or `display.blueLedPin()` to toggle the onboard LED from UI switches or buttons (active-low: `LOW` = ON).
