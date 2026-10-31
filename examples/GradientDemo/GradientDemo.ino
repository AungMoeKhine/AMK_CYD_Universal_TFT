/*
  AMK_CYD_Universal_TFT - Gradient & Alpha Blending Demo
  ------------------------------------------------------
  Showcases the high-performance linear color gradients and alpha-blending
  features introduced in AMK_CYD_Universal_TFT v1.4.0:

    1. fillRectHGradient() - Horizontal linear color gradient
    2. fillRectVGradient() - Vertical linear color gradient
    3. alphaBlend()        - High-speed 16-bit RGB565 alpha blending
    4. TFT_eSPI_Button     - Interactive touch button with theme toggling
    5. Onboard RGB LED     - Hardware sync matching the active theme

  Hardware Supported:
    ESP32 Cheap Yellow Display (CYD) resistive boards
    (2.4", 2.8" ILI9341/ST7789, 3.2", 3.5" ST7796, and Dual-USB)

  Author: Aung Moe Khine
  Repository: https://github.com/aungmoekhine/AMK_CYD_Universal_TFT
*/

#include <AMK_CYD_Universal_TFT.h>

// Instantiate display, touch, and TFT_eSPI instance
AMK_CYD_display display;
AMK_CYD_touch   touch;
TFT_eSPI        tft;
TFT_eSPI_Button btnTheme;

// Theme definitions
struct GradientTheme {
    const char* name;
    uint16_t headerStart;
    uint16_t headerEnd;
    uint16_t card1Top;
    uint16_t card1Bot;
    uint16_t card2Top;
    uint16_t card2Bot;
    uint16_t card3Top;
    uint16_t card3Bot;
    uint16_t btnColor;
    uint8_t  ledMask; // 1=Red, 2=Green, 4=Blue
};

const GradientTheme themes[] = {
    // 0: Cyberpunk Neon
    {
        "CYBERPUNK NEON",
        0x5010, TFT_CYAN,        // Header: Deep Magenta -> Cyan
        0xF81F, 0x3008,          // Card 1: Magenta -> Dark Purple
        0x07FF, 0x0010,          // Card 2: Cyan -> Deep Navy
        0xFFE0, 0x4200,          // Card 3: Yellow -> Deep Amber
        0x5015,
        4                        // Blue LED
    },
    // 1: Sunset Glow
    {
        "SUNSET GLOW",
        0x7800, 0xFD20,          // Header: Maroon -> Bright Orange
        0xF800, 0x3800,          // Card 1: Red -> Dark Red
        0xFD20, 0x5100,          // Card 2: Orange -> Dark Brown
        0xFFE0, 0x6200,          // Card 3: Yellow -> Rust
        0x8900,
        1                        // Red LED
    },
    // 2: Deep Ocean
    {
        "DEEP OCEAN",
        0x000F, 0x07F5,          // Header: Midnight Blue -> Vivid Teal
        0x03FF, 0x000C,          // Card 1: Sky Blue -> Abyss
        0x07E8, 0x01E0,          // Card 2: Emerald -> Forest Green
        0x067F, 0x0188,          // Card 3: Teal -> Deep Sea
        0x028A,
        2                        // Green LED
    }
};

const uint8_t THEME_COUNT = sizeof(themes) / sizeof(themes[0]);
uint8_t currentTheme = 0;

void setOnboardRGB(uint8_t mask) {
    // CYD RGB LED is active-LOW
    digitalWrite(display.redLedPin(),   (mask & 1) ? LOW : HIGH);
    digitalWrite(display.greenLedPin(), (mask & 2) ? LOW : HIGH);
    digitalWrite(display.blueLedPin(),  (mask & 4) ? LOW : HIGH);
}

void drawGlassBar(int16_t x, int16_t y, int16_t w, int16_t h) {
    // Draw background gradient underlying bar
    tft.fillRectHGradient(x, y, w, h, TFT_BLACK, 0x2124);

    // Demonstrate alphaBlend: 4 glass-morphism panels with 25%, 50%, 75%, 100% alpha
    uint8_t alphas[4] = { 64, 128, 192, 255 };
    const char* labels[4] = { "25%", "50%", "75%", "100%" };
    int16_t sw = (w - 20) / 4;

    for (int i = 0; i < 4; i++) {
        int16_t sx = x + 4 + i * (sw + 4);
        // Alpha blend pure white over dark background
        uint16_t blended = tft.alphaBlend(alphas[i], TFT_WHITE, 0x18E3);
        tft.fillRoundRect(sx, y + 3, sw, h - 6, 3, blended);
        tft.drawRoundRect(sx, y + 3, sw, h - 6, 3, TFT_WHITE);

        tft.setTextDatum(MC_DATUM);
        tft.setTextColor((alphas[i] > 128) ? TFT_BLACK : TFT_WHITE, blended);
        tft.drawString(labels[i], sx + (sw / 2), y + (h / 2), 1);
    }
}

void renderDashboard() {
    const GradientTheme &th = themes[currentTheme];

    // Update hardware LED
    setOnboardRGB(th.ledMask);

    // 1. Top Header with Horizontal Linear Gradient
    tft.fillRectHGradient(0, 0, tft.width(), 32, th.headerStart, th.headerEnd);
    tft.drawFastHLine(0, 32, tft.width(), TFT_WHITE);

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_TRANSPARENT);
    tft.drawString("AMK GRADIENT & ALPHA DEMO", tft.width() / 2, 16, 2);

    // 2. Three Metric Cards with Vertical Linear Gradients
    // Dimensions: 3 cards x 96px width + 8px gaps = 320px
    const int16_t cardW = 96;
    const int16_t cardH = 94;
    const int16_t cardY = 38;

    // Card 1: CPU / Core
    int16_t c1X = 8;
    tft.fillRectVGradient(c1X, cardY, cardW, cardH, th.card1Top, th.card1Bot);
    tft.drawRect(c1X, cardY, cardW, cardH, TFT_WHITE);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_TRANSPARENT);
    tft.drawString("CORE TEMP", c1X + (cardW / 2), cardY + 6, 1);
    tft.drawString("48.5 C", c1X + (cardW / 2), cardY + 28, 2);
    tft.drawString("NORMAL", c1X + (cardW / 2), cardY + 70, 1);

    // Card 2: Wireless Signal
    int16_t c2X = 112;
    tft.fillRectVGradient(c2X, cardY, cardW, cardH, th.card2Top, th.card2Bot);
    tft.drawRect(c2X, cardY, cardW, cardH, TFT_WHITE);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_TRANSPARENT);
    tft.drawString("WIFI RSSI", c2X + (cardW / 2), cardY + 6, 1);
    tft.drawString("-52 dBm", c2X + (cardW / 2), cardY + 28, 2);
    tft.drawString("EXCELLENT", c2X + (cardW / 2), cardY + 70, 1);

    // Card 3: Battery Telemetry
    int16_t c3X = 216;
    tft.fillRectVGradient(c3X, cardY, cardW, cardH, th.card3Top, th.card3Bot);
    tft.drawRect(c3X, cardY, cardW, cardH, TFT_WHITE);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_TRANSPARENT);
    tft.drawString("BATTERY", c3X + (cardW / 2), cardY + 6, 1);
    tft.drawString("96 %", c3X + (cardW / 2), cardY + 28, 2);
    tft.drawString("CHARGING", c3X + (cardW / 2), cardY + 70, 1);

    // 3. Alpha Blended Frosted Glass Strip
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("Alpha Blended Transparency (25% - 100%):", 8, 138, 1);
    drawGlassBar(8, 150, 304, 28);

    // 4. Interactive Theme Switch Button (TFT_eSPI_Button)
    char labelBuf[40];
    snprintf(labelBuf, sizeof(labelBuf), "THEME: %s (TAP TO CYCLE)", th.name);
    btnTheme.initButtonUL(&tft, 8, 186, 304, 34, TFT_WHITE, th.btnColor, TFT_WHITE, labelBuf, 1);
    btnTheme.drawButton(false);

    // Footer Info
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("AMK_CYD_Universal_TFT v1.4.0 * Native Hardware Gradients", tft.width() / 2, 230, 1);
}

void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println("\n--- AMK_CYD_Universal_TFT Gradient Demo ---");

    // Initialize display and touch with runtime hardware auto-probing
    tft.init();
    tft.setRotation(3); // Standard CYD landscape rotation
    tft.fillScreen(TFT_BLACK);

    // Initialize onboard RGB LED pins as outputs
    pinMode(display.redLedPin(), OUTPUT);
    pinMode(display.greenLedPin(), OUTPUT);
    pinMode(display.blueLedPin(), OUTPUT);

    // Render the initial dashboard
    renderDashboard();
}

void loop() {
    uint16_t t_x = 0, t_y = 0;
    bool touched = tft.getTouch(&t_x, &t_y);

    // Update button press state
    btnTheme.press(touched && btnTheme.contains(t_x, t_y));

    // Handle touch press feedback
    if (btnTheme.justPressed()) {
        btnTheme.drawButton(true); // Invert button colors for tactile feedback
    }

    // Handle touch release: cycle to next gradient palette
    if (btnTheme.justReleased()) {
        currentTheme = (currentTheme + 1) % THEME_COUNT;
        tft.fillScreen(TFT_BLACK);
        renderDashboard();
    }

    delay(15);
}
