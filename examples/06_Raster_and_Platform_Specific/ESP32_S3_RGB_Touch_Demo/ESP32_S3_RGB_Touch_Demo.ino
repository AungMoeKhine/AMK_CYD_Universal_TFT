// ═══════════════════════════════════════════════════════════════════════════════
// ESP32_S3_RGB_Touch_Demo.ino
//
// Universal ESP32-S3 RGB Display & Capacitive Touch Demo for AMK_CYD_Universal_TFT
// Supports Waveshare & Sunton 4.3", 5.0", 7.0" (ST7262 + GT911 capacitive touch)
//
// Hardware Settings in Arduino IDE (Tools menu):
//   - Board: "Waveshare ESP32-S3-Touch-LCD-7" or "ESP32S3 Dev Module"
//   - PSRAM: "OPI PSRAM" / "Enabled" (Required: framebuffers live in PSRAM)
//   - Flash Size: "8MB (64Mb)" or "16MB"
//   - USB CDC On Boot: "Enabled"
// ═══════════════════════════════════════════════════════════════════════════════

#include <AMK_CYD_Universal_TFT.h>

// Global hardware instances (default board controller handles CH422G expander)
AMK_S3_Board        board;
AMK_S3_RGB_display  display(board);
AMK_S3_touch        touch(board);
TFT_eSPI            tft;

// GUI Elements
TFT_eSPI_Button btn1;
TFT_eSPI_Button btn2;
TFT_eSPI_Button btn3;

int counter = 0;
uint16_t lastTouchX = 0;
uint16_t lastTouchY = 0;
bool wasTouched = false;

static int cardX = 40;
static int cardY = 240;
static int cardW = 720;
static int cardH = 200;

void drawCounter() {
    tft.setTextSize(1);
    char buf[32];
    snprintf(buf, sizeof(buf), "Counter: %d", counter);
    int cntX = (tft.width() - 220) / 2;
    int cntY = cardY + cardH - 55;
    tft.fillRect(cntX, cntY, 220, 42, TFT_BLACK);
    tft.setFreeFont(&FreeSansBold12pt7b);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(buf, tft.width() / 2, cntY + 21);
    tft.setFreeFont(nullptr);
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n==========================================");
    Serial.println(" AMK Universal TFT - ESP32-S3 RGB Touch Demo");
    Serial.println("==========================================");

    // Initialize display and capacitive touch controller
    tft.init();
    tft.setRotation(0); // Native landscape for S3 RGB panels (800x480)

    int sw = tft.width();
    int sh = tft.height();
    Serial.printf("Display initialized: %d x %d\n", sw, sh);

    // Draw header background with gradient
    int headerH = (sh >= 400) ? 60 : 40;
    tft.drawGradientH(0, 0, sw, headerH, TFT_NAVY, TFT_DARKCYAN);
    tft.setFreeFont(&FreeSansBold12pt7b);
    tft.setTextColor(TFT_WHITE, TFT_TRANSPARENT);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("AMK Universal RGB Display Demo", sw / 2, headerH / 2);
    tft.setFreeFont(nullptr);

    // Draw main background
    tft.fillRect(0, headerH, sw, sh - headerH, TFT_BLACK);

    // Responsive button calculations
    int btnW = (sw - 100) / 3;
    int btnH = (sh >= 400) ? 60 : 44;
    int btnY = headerH + 30 + (btnH / 2);

    int b1X = 25 + (btnW / 2);
    int b2X = sw / 2;
    int b3X = sw - 25 - (btnW / 2);

    // Initialize interactive touch buttons
    btn1.initButton(&tft, b1X, btnY, btnW, btnH, TFT_WHITE, TFT_BLUE,      TFT_WHITE, "Count Up", 2);
    btn2.initButton(&tft, b2X, btnY, btnW, btnH, TFT_WHITE, TFT_DARKGREEN, TFT_WHITE, "Reset",    2);
    btn3.initButton(&tft, b3X, btnY, btnW, btnH, TFT_WHITE, TFT_RED,       TFT_WHITE, "Invert",   2);

    btn1.drawButton();
    btn2.drawButton();
    btn3.drawButton();
    tft.setTextSize(1);

    // Responsive info card
    cardX = 25;
    cardY = btnY + (btnH / 2) + 20;
    cardW = sw - 50;
    cardH = sh - cardY - 20;

    tft.drawRoundRect(cardX, cardY, cardW, cardH, 10, TFT_DARKGREY);
    tft.setFreeFont(&FreeSansBold9pt7b);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Capacitive Multi-Touch (GT911) Live Test", sw / 2, cardY + 28);
    tft.setFreeFont(nullptr);
    tft.drawString("Tap buttons or touch anywhere to see coordinates", sw / 2, cardY + 58, 1);

    drawCounter();
}

void loop() {
    uint16_t tx = 0, ty = 0;
    bool touched = tft.getTouch(&tx, &ty);

    // Update buttons
    btn1.press(touched && btn1.contains(tx, ty));
    btn2.press(touched && btn2.contains(tx, ty));
    btn3.press(touched && btn3.contains(tx, ty));

    if (btn1.justPressed()) {
        btn1.drawButton(true);
        tft.setTextSize(1);
        counter++;
        drawCounter();
    }
    if (btn1.justReleased()) {
        btn1.drawButton(false);
        tft.setTextSize(1);
    }

    if (btn2.justPressed()) {
        btn2.drawButton(true);
        tft.setTextSize(1);
        counter = 0;
        drawCounter();
    }
    if (btn2.justReleased()) {
        btn2.drawButton(false);
        tft.setTextSize(1);
    }

    if (btn3.justPressed()) {
        btn3.drawButton(true);
        tft.setTextSize(1);
        static bool inv = false;
        inv = !inv;
        tft.invertDisplay(inv);
    }
    if (btn3.justReleased()) {
        btn3.drawButton(false);
        tft.setTextSize(1);
    }

    // Live touch feedback cursor
    if (touched) {
        if (!wasTouched || abs((int)tx - (int)lastTouchX) > 4 || abs((int)ty - (int)lastTouchY) > 4) {
            // Draw touch circle
            tft.fillCircle(tx, ty, 6, TFT_CYAN);
            lastTouchX = tx;
            lastTouchY = ty;
            wasTouched = true;
        }
    } else {
        wasTouched = false;
    }

    delay(10);
}
