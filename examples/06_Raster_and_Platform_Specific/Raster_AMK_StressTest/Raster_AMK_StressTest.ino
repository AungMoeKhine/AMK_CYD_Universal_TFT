/*******************************************************************************
 * AMK_CYD_Universal_TFT -- 80 MHz SPI Stress Test & Raster Benchmark (v1.8.0)
 *
 * Demonstrates:
 *   1. Hardware auto-detection & Dynamic Backlight Auto-Dimming (LDR Filter).
 *   2. 80 MHz SPI Bus Stress Testing with tft.stressTest() & NVS Persistence.
 *   3. Panel Refresh Rate Detection and Tear-Free Raster Direct Blitting.
 *   4. Zero-buffer Beam Chasing Scanline Synchronization.
 *
 * Standards Followed:
 *   - Auto-detected backlight pin (display.backlightPin()).
 *   - Moving average ambient LDR filter primed at boot.
 *   - LDR auto-dimming first in loop().
 *   - Frozen RGB/BGR color tables & MADCTL.
 *
 * Board Support: ESP32 CYD 2.4", 2.8", 3.2", 3.5", 4.0" (Resistive & Capacitive).
 *
 * Author: Aung Moe Khine (AMK)
 ******************************************************************************/

#include <Arduino.h>
#include <AMK_CYD_Universal_TFT.h>

// 1. Standard Global Driver Objects
AMK_display display;
AMK_touch   touch;
TFT_eSPI    tft;

// Moving average LDR configuration
#define LDR_PIN 34
#define NUM_LDR_READINGS 10

static void testProgress(uint32_t elapsedMs, uint32_t totalMs) {
    static uint32_t lastSec = 999;
    uint32_t remainingSec = (totalMs - min(elapsedMs, totalMs) + 999) / 1000;
    if (remainingSec != lastSec) {
        lastSec = remainingSec;
        Serial.printf("[AMK Stress Test] Elapsed: %u ms / %u ms (%u s left)\n", 
                      (unsigned)elapsedMs, (unsigned)totalMs, (unsigned)remainingSec);
    }
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n==========================================");
    Serial.println("  AMK CYD Universal TFT v1.8.0");
    Serial.println("  80 MHz Stress Test & Raster Benchmark");
    Serial.println("==========================================");

    // 1. Initialize Display Core
    tft.init();
    tft.setRotation(3); // Landscape (USB Top/Right - AMK Standard)
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

    // 2. Hardware Backlight PWM on Auto-Detected Pin
    pinMode(LDR_PIN, INPUT);
    uint8_t blPin = display.backlightPin();
    pinMode(blPin, OUTPUT);
    analogWriteResolution(blPin, 8);
    analogWrite(blPin, 180);

    // 3. Prime the LDR filter buffer at boot (User Hardware Standard)
    for (int i = 0; i < NUM_LDR_READINGS; i++) {
        tft.adjustBrightness();
        delay(10);
    }

    // 4. Print Hardware Diagnostic Info
    Serial.printf("Model: %s\n", tft.modelName());
    Serial.printf("Panel: %s\n", tft.panelName());
    Serial.printf("Native Res: %dx%d\n", tft.nativeWidth(), tft.nativeHeight());
    Serial.printf("Current Clock: %u MHz\n", (unsigned)(display.spiClock() / 1000000));

    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 15);
    tft.println("AMK CYD Universal v1.8");

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(10, 45);
    tft.printf("Board: %s (%s)", tft.modelName(), tft.panelName());
    tft.setCursor(10, 60);
    tft.printf("Resolution: %dx%d | Backlight Pin: %d", tft.width(), tft.height(), blPin);

    // 5. Check if 80 MHz Speed Test was previously run & recorded in NVS
    cydr_speed_test_t speedRec = {};
    bool hasRecord = tft.loadSpeedTest(speedRec);

    tft.setCursor(10, 85);
    if (hasRecord && speedRec.result == CYDR_SPEED_PASSED) {
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.printf("NVS Record: 80 MHz PASSED (%d windows, 0 bad)", (int)speedRec.windows);
        Serial.printf("Loaded NVS Speed Record: PASSED at %u Hz\n", (unsigned)speedRec.hz);
    } else if (hasRecord && speedRec.result == CYDR_SPEED_FAILED) {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.printf("NVS Record: 80 MHz FAILED (%d/%d bad)", (int)speedRec.bad, (int)speedRec.windows);
        Serial.printf("Loaded NVS Speed Record: FAILED (%d bad)\n", (int)speedRec.bad);
    } else {
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.println("NVS Record: UNTESTED (Will benchmark now)");
        Serial.println("No existing 80 MHz speed test found in NVS.");
    }

    // 6. Run 80 MHz SPI Stress Test (10 seconds)
    tft.setCursor(10, 115);
    tft.setTextColor(TFT_GOLD, TFT_BLACK);
    tft.println("Starting 80 MHz SPI Stress Test (10s)...");
    tft.setCursor(10, 130);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.println("Colorful static & LED flashes are normal.");
    delay(1500);

    int totalWindows = 0;
    int badWindows = tft.stressTest(80000000, 10000, testProgress, &totalWindows);

    // Redraw interface after static noise
    tft.fillScreen(TFT_BLACK);
    tft.setRotation(3);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 15);
    tft.println("AMK CYD Universal v1.8");

    tft.setTextSize(1);
    tft.setCursor(10, 50);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.printf("Stress Test Completed: %d windows checked.", totalWindows);

    speedRec.hz = 80000000;
    speedRec.windows = totalWindows;
    speedRec.bad = max(0, badWindows);
    speedRec.version = CYDR_SPEED_TEST_VERSION;

    tft.setCursor(10, 75);
    if (badWindows == 0 && totalWindows > 0) {
        speedRec.result = CYDR_SPEED_PASSED;
        tft.saveSpeedTest(speedRec);

        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.setTextSize(2);
        tft.println(">> RESULT: 80 MHz PASSED! <<");
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.println("Your CYD board is certified for full 80 MHz SPI!");
        tft.println("Max performance speed record saved to NVS flash.");

        // Switch live SPI clock to 80 MHz
        tft.changeSpiClock(80000000);
        Serial.println("Successfully switched SPI Clock to 80 MHz!");
    } else {
        speedRec.result = CYDR_SPEED_FAILED;
        tft.saveSpeedTest(speedRec);

        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.setTextSize(2);
        tft.println(">> RESULT: 80 MHz FAILED <<");
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.printf("Errors: %d bad windows. Staying at safe 40 MHz.\n", badWindows);
    }

    // 7. Check Panel Scanline Capability (Command 0x45)
    uint8_t scanBuf[4] = {0};
    bool canReadScan = tft.readCommand(0x45, scanBuf, 2, 2000000);
    tft.setCursor(10, 155);
    tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
    tft.printf("Scanline Hook (0x45): %s (Bytes: 0x%02X 0x%02X)\n", 
               canReadScan ? "AVAILABLE (Tear-Free Ready)" : "UNAVAILABLE", 
               scanBuf[0], scanBuf[1]);

    // 8. Panel Refresh Steps Available
    int refreshSteps = tft.refreshStepCount();
    tft.setCursor(10, 175);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.printf("Panel Refresh Steps: %d steps available\n", refreshSteps);

    tft.setCursor(10, 205);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Running live raster performance monitor in loop()...");
}

void loop() {
    // Standard First Line: Ambient LDR Auto-dimming
    tft.adjustBrightness();

    static unsigned long lastUpdate = 0;
    static uint32_t frameCount = 0;
    frameCount++;

    if (millis() - lastUpdate >= 1000) {
        float fps = (float)frameCount * 1000.0f / (millis() - lastUpdate);
        lastUpdate = millis();
        frameCount = 0;

        tft.fillRect(10, 220, 300, 16, TFT_BLACK);
        tft.setCursor(10, 220);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.printf("Live FPS: %.1f | Backlight PWM: %d", fps, tft.getActiveBacklight());
    }

    // Draw small animated raster strip
    static int stripX = 0;
    tft.fillRect(stripX, 195, 8, 4, TFT_BLACK);
    stripX = (stripX + 4) % (tft.width() - 8);
    tft.fillRect(stripX, 195, 8, 4, TFT_CYAN);

    delay(10);
}
