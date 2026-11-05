/*
  AMK_CYD_Universal_TFT - Gradient & Alpha Blending Demo
  ------------------------------------------------------
  Showcases high-performance linear color gradients and alpha-blending:

    1. fillRectHGradient() - Horizontal linear color gradient
    2. fillRectVGradient() - Vertical linear color gradient
    3. alphaBlend()        - High-speed 16-bit RGB565 alpha blending
    4. TFT_eSPI_Button     - Interactive touch button with theme toggling
    5. Onboard RGB LED     - Hardware sync matching the active theme
    6. Balanced FreeFonts  - Perfectly centered alignment & symmetrical margins

  Hardware Supported:
    All Cheap Yellow Display (CYD) boards (2.4", 2.8", 3.2", 3.5", 4.0")
    resistive & capacitive, with standard Rotation 3 landscape.
*/

#include <AMK_CYD_Universal_TFT.h>

// Instantiate display, touch, and TFT_eSPI instance
AMK_display display;
AMK_touch   touch;
TFT_eSPI    tft;
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
#if !defined(AMK_BACKEND_ESP32_S3)
    // CYD RGB LED is active-LOW
    digitalWrite(display.redLedPin(),   (mask & 1) ? LOW : HIGH);
    digitalWrite(display.greenLedPin(), (mask & 2) ? LOW : HIGH);
    digitalWrite(display.blueLedPin(),  (mask & 4) ? LOW : HIGH);
#endif
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
    int sw = tft.width();
    int sh = tft.height();

    // Update hardware LED
    setOnboardRGB(th.ledMask);

    // 1. Top Header with Horizontal Linear Gradient and centered FreeFont
    tft.fillRectHGradient(0, 0, sw, 30, th.headerStart, th.headerEnd);
    tft.drawFastHLine(0, 30, sw, TFT_WHITE);

    const char* title = (sw >= 480) ? "AMK GRADIENT & ALPHA DEMO" : "AMK GRADIENT DEMO";
    tft.setFreeFont(&FreeSansBold9pt7b);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_TRANSPARENT);
    tft.drawString(title, sw / 2, 15);

    // 2. Three Metric Cards with Symmetrical Gaps & Margins
    int16_t gap         = (sw >= 480) ? 10 : 8;
    int16_t totalCardsW = sw - (gap * 4);
    int16_t cardW       = totalCardsW / 3;
    int16_t rem         = totalCardsW % 3;
    int16_t sideMargin  = gap + (rem / 2); // Equal left & right margin

    int16_t cardH = (sh >= 320) ? 110 : 80;
    int16_t cardY = 36;

    int16_t c1X = sideMargin;
    int16_t c2X = sideMargin + cardW + gap;
    int16_t c3X = sideMargin + (cardW + gap) * 2;

    // Card 1: CPU / Core
    tft.fillRectVGradient(c1X, cardY, cardW, cardH, th.card1Top, th.card1Bot);
    tft.drawRect(c1X, cardY, cardW, cardH, TFT_WHITE);
    tft.setFreeFont(nullptr);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_TRANSPARENT);
    tft.drawString("CORE TEMP", c1X + (cardW / 2), cardY + 12, 1);

    tft.setFreeFont(&FreeSansBold9pt7b);
    tft.drawString("48.5 C", c1X + (cardW / 2), cardY + 40);
    tft.setFreeFont(nullptr);
    tft.drawString("NORMAL", c1X + (cardW / 2), cardY + cardH - 12, 1);

    // Card 2: Wireless Signal
    tft.fillRectVGradient(c2X, cardY, cardW, cardH, th.card2Top, th.card2Bot);
    tft.drawRect(c2X, cardY, cardW, cardH, TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_TRANSPARENT);
    tft.drawString("WIFI RSSI", c2X + (cardW / 2), cardY + 12, 1);

    tft.setFreeFont(&FreeSansBold9pt7b);
    tft.drawString("-52 dBm", c2X + (cardW / 2), cardY + 40);
    tft.setFreeFont(nullptr);
    tft.drawString("EXCELLENT", c2X + (cardW / 2), cardY + cardH - 12, 1);

    // Card 3: Battery Telemetry
    tft.fillRectVGradient(c3X, cardY, cardW, cardH, th.card3Top, th.card3Bot);
    tft.drawRect(c3X, cardY, cardW, cardH, TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_TRANSPARENT);
    tft.drawString("BATTERY", c3X + (cardW / 2), cardY + 12, 1);

    tft.setFreeFont(&FreeSansBold9pt7b);
    tft.drawString("96 %", c3X + (cardW / 2), cardY + 40);
    tft.setFreeFont(nullptr);
    tft.drawString("CHARGING", c3X + (cardW / 2), cardY + cardH - 12, 1);

    // 3. Alpha Blended Frosted Glass Strip with clear spacing
    int16_t glassLabelY = cardY + cardH + 9;
    int16_t glassH      = (sh >= 320) ? 28 : 24;
    int16_t glassBarY   = glassLabelY + 9;
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("Alpha Blended Transparency (25% - 100%)", sw / 2, glassLabelY, 1);
    drawGlassBar(sideMargin, glassBarY, sw - (sideMargin * 2), glassH);

    // 4. Interactive Theme Switch Button (Symmetrically aligned)
    int16_t btnY = glassBarY + glassH + 9;
    int16_t btnH = (sh >= 320) ? 38 : 32;
    char labelBuf[48];
    snprintf(labelBuf, sizeof(labelBuf), "THEME: %s (TAP TO CYCLE)", th.name);
    btnTheme.initButtonUL(&tft, sideMargin, btnY, sw - (sideMargin * 2), btnH, TFT_WHITE, th.btnColor, TFT_WHITE, labelBuf, 1);
    btnTheme.drawButton(false);

    // Footer Info (Centered)
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("AMK_CYD_Universal_TFT Native Hardware Gradients", sw / 2, sh - 7, 1);
}

void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println("\n--- AMK_CYD_Universal_TFT Gradient Demo ---");

    // Initialize display & touch with Rotation 3 landscape
    tft.init();
    tft.setRotation(3);
    tft.setBacklight(100);
    tft.fillScreen(TFT_BLACK);

    touch.begin(display);
    touch.setRotation(3);
    if (!touch.capacitive()) {
        touch.loadCalibration();
        touch.loadFilters();
    }

    // Initialize onboard RGB LED pins as outputs
#if !defined(AMK_BACKEND_ESP32_S3)
    pinMode(display.redLedPin(), OUTPUT);
    pinMode(display.greenLedPin(), OUTPUT);
    pinMode(display.blueLedPin(), OUTPUT);
#endif

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
