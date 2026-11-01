// ═══════════════════════════════════════════════════════════════════════════════
// ESP32_S3_RGB_Touch_Demo.ino
//
// Universal ESP32-S3 RGB Display & Capacitive Touch Demo for AMK_CYD_Universal_TFT
// Supports Waveshare & Sunton 4.3", 5.0", 7.0" (800x480 ST7262 + GT911 capacitive touch)
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
// You can use AMK_S3_RGB_display or chipguy_S3_RGB_display
AMK_S3_RGB_display  display(board);
// You can use AMK_S3_touch or chipguy_S3_capacitive_touch
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

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n==========================================");
    Serial.println(" AMK Universal TFT - ESP32-S3 RGB Touch Demo");
    Serial.println("==========================================");

    // Initialize display and capacitive touch controller
    tft.init();
    tft.setRotation(0); // Landscape: 800x480

    Serial.printf("Display initialized: %d x %d\n", tft.width(), tft.height());

    // Draw header background with gradient
    tft.drawGradientH(0, 0, tft.width(), 60, TFT_NAVY, TFT_DARKCYAN);
    tft.setTextColor(TFT_WHITE, TFT_TRANSPARENT);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("AMK ESP32-S3 Universal RGB Display (800x480)", tft.width() / 2, 30, 4);

    // Draw main background
    tft.fillRect(0, 60, tft.width(), tft.height() - 60, TFT_BLACK);

    // Initialize interactive touch buttons
    btn1.initButton(&tft, 160, 160, 180, 60, TFT_WHITE, TFT_BLUE, TFT_WHITE, "Count Up", 2);
    btn2.initButton(&tft, 400, 160, 180, 60, TFT_WHITE, TFT_DARKGREEN, TFT_WHITE, "Reset", 2);
    btn3.initButton(&tft, 640, 160, 180, 60, TFT_WHITE, TFT_RED, TFT_WHITE, "Invert", 2);

    btn1.drawButton();
    btn2.drawButton();
    btn3.drawButton();

    // Draw info card
    tft.drawRoundRect(50, 240, 700, 200, 12, TFT_DARKGREY);
    tft.drawString("Capacitive Multi-Touch (GT911) Live Test", 400, 270, 4);
    tft.drawString("Tap buttons or touch anywhere to see coordinates", 400, 310, 2);

    drawCounter();
}

void drawCounter() {
    char buf[32];
    snprintf(buf, sizeof(buf), "Counter: %d", counter);
    tft.fillRect(300, 350, 200, 40, TFT_BLACK);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(buf, 400, 370, 4);
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
        counter++;
        drawCounter();
    }
    if (btn1.justReleased()) {
        btn1.drawButton(false);
    }

    if (btn2.justPressed()) {
        btn2.drawButton(true);
        counter = 0;
        drawCounter();
    }
    if (btn2.justReleased()) {
        btn2.drawButton(false);
    }

    if (btn3.justPressed()) {
        btn3.drawButton(true);
        static bool inv = false;
        inv = !inv;
        tft.invertDisplay(inv);
    }
    if (btn3.justReleased()) {
        btn3.drawButton(false);
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
