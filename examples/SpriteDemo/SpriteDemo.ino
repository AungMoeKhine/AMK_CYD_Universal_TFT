// ═══════════════════════════════════════════════════════════════════════════════
// SpriteDemo.ino - Demonstration of TFT_eSprite in AMK_CYD_Universal_TFT
//
// Shows off-screen buffering in:
//   1. 16-bit color depth (RGB565 full color)
//   2. 8-bit color depth (RGB332 low RAM)
//   3. 4-bit color depth with palette (minimal RAM: ~28KB for 240x240 analog clock)
// ═══════════════════════════════════════════════════════════════════════════════

#include <Arduino.h>
#include <AMK_CYD_Universal_TFT.h>

// Driver objects
AMK_CYD_display display;
AMK_CYD_touch   touch;
TFT_eSPI        tft;

// Sprites
TFT_eSprite sprite16 = TFT_eSprite(&tft);
TFT_eSprite sprite8  = TFT_eSprite(&tft);
TFT_eSprite sprite4  = TFT_eSprite(&tft);

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[AMK_CYD_Universal_TFT] Sprite Demo");

    // 1. Initialise TFT display (auto-detects CYD panel & wiring)
    tft.init();
    tft.setRotation(1); // Landscape
    tft.fillScreen(TFT_BLACK);

    // 2. Initialise Touch controller
    touch.begin(display);
    touch.setRotation(1);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("AMK CYD Universal Sprite Demo", tft.width() / 2, 5);

    // ── Test 1: 16-bit Sprite (80x60) ──
    sprite16.setColorDepth(16);
    sprite16.createSprite(80, 60);
    if (sprite16.created()) {
        sprite16.fillSprite(TFT_NAVY);
        sprite16.drawRect(0, 0, 80, 60, TFT_CYAN);
        sprite16.setTextColor(TFT_YELLOW, TFT_NAVY);
        sprite16.setTextDatum(MC_DATUM);
        sprite16.drawString("16-Bit", 40, 20);
        sprite16.drawString("Sprite", 40, 40);
        sprite16.pushSprite(10, 30);
    }

    // ── Test 2: 8-bit Sprite (80x60) ──
    sprite8.setColorDepth(8);
    sprite8.createSprite(80, 60);
    if (sprite8.created()) {
        sprite8.fillSprite(TFT_DARKGREEN);
        sprite8.drawRect(0, 0, 80, 60, TFT_GREEN);
        sprite8.fillCircle(40, 30, 20, TFT_RED);
        sprite8.setTextColor(TFT_WHITE, TFT_RED);
        sprite8.setTextDatum(MC_DATUM);
        sprite8.drawString("8-Bit", 40, 30);
        sprite8.pushSprite(105, 30);
    }

    // ── Test 3: 4-bit Palette Sprite (80x60) ──
    sprite4.setColorDepth(4);
    sprite4.createSprite(80, 60);
    if (sprite4.created()) {
        uint16_t cmap[16] = {
            TFT_BLACK, TFT_WHITE, TFT_RED,    TFT_GREEN,
            TFT_BLUE,  TFT_YELLOW, TFT_MAGENTA, TFT_CYAN,
            0x7BEF,    0xFD20,    0xAFE5,     0x000F,
            0x03E0,    0x7800,    0x780F,     TFT_WHITE
        };
        sprite4.createPalette(cmap);
        sprite4.fillSprite(0); // Index 0: Black
        sprite4.drawRect(0, 0, 80, 60, 7); // Index 7: Cyan
        sprite4.fillTriangle(40, 5, 10, 55, 70, 55, 5); // Index 5: Yellow
        sprite4.setTextColor(2, 0); // Index 2: Red
        sprite4.setTextDatum(MC_DATUM);
        sprite4.drawString("4-Bit", 40, 38);
        sprite4.pushSprite(200, 30);
    }
}

void loop() {
    // Dynamic animation on 16-bit sprite to show smooth frame rendering
    static int angle = 0;
    static unsigned long lastTick = 0;

    if (millis() - lastTick >= 50) {
        lastTick = millis();
        angle = (angle + 6) % 360;

        if (sprite16.created()) {
            sprite16.fillSprite(TFT_BLACK);
            sprite16.drawCircle(40, 30, 25, TFT_DARKGREY);

            float rad = angle * 0.0174532925;
            int xTip = 40 + cos(rad) * 22;
            int yTip = 30 + sin(rad) * 22;
            sprite16.drawLine(40, 30, xTip, yTip, TFT_GREEN);
            sprite16.fillCircle(40, 30, 3, TFT_RED);

            sprite16.pushSprite(10, 110);
        }
    }

    // Touch feedback
    uint16_t tx, ty;
    if (tft.getTouch(&tx, &ty)) {
        tft.fillCircle(tx, ty, 3, TFT_YELLOW);
    }
}
