/*
 * SpriteDemo.ino - AMK_CYD_Universal_TFT Flagship Interactive Showcase
 *
 * Fully responsive for ALL Cheap Yellow Display (CYD) screen sizes:
 *   - 2.4", 2.8", 3.2" (240x320 portrait / 320x240 landscape)
 *   - 3.5", 4.0" (320x480 portrait / 480x320 landscape)
 *
 * Demonstrates:
 *  1. Crisp PROGMEM RGB565 logo image rendering (AMK Logo)
 *  2. Flicker-free high-speed off-screen sprites (TFT_eSprite)
 *  3. Dynamic analog arc gauge with animated needle and real-time telemetry
 *  4. Interactive touch & drag slider controlling live LCD backlight
 *  5. Real-time oscilloscope-style scrolling waveform graph with speed controls
 *  6. Onboard RGB LED hardware controller (GPIO 4/22 and 16/17)
 *  7. Live LDR light sensor meter (GPIO 34 ADC1)
 *  8. Touch coordinate tracking with responsive UI feedback
 */

#include <Arduino.h>
#include <AMK_CYD_Universal_TFT.h>
#include <math.h>
#include "logo_tft.h"

// ── 1. GLOBAL DRIVER OBJECTS ──────────────────────────────────────────────────
AMK_CYD_display display;
AMK_CYD_touch   touch;
TFT_eSPI        tft;

// Off-screen Sprites in RAM
TFT_eSprite gaugeSprite = TFT_eSprite(&tft);  // 124x124 16-bit analog meter
TFT_eSprite waveSprite  = TFT_eSprite(&tft);  // 270x110 16-bit oscilloscope wave

// ── 2. HIGH-TECH COLOR PALETTE ────────────────────────────────────────────────
#define C_DARK_BG    0x0000
#define C_HEADER_BG  0x10A2
#define C_CARD_BG    0x18E3
#define C_BORDER     0x2965
#define C_TAB_SEL    0x0215
#define C_TAB_UNSEL  0x10A2
#define C_CYAN_ACC   0x07FF
#define C_GREEN_ACC  0x07E0
#define C_YELLOW_ACC 0xFFE0
#define C_RED_ACC    0xF800
#define C_TEXT_WHITE 0xFFFF
#define C_TEXT_MUTED 0x94B2

// ── 3. STATE & TELEMETRY ──────────────────────────────────────────────────────
static int currentTab = 0;       // 0: METER, 1: WAVE, 2: RGB/LDR, 3: LOGO
static int backlightPct = 100;   // 10 - 100%

// Dynamic screen geometry (computed after tft.init())
static int SCRW = 320;
static int SCRH = 240;
static int BOTY = 222;

// Oscilloscope state
#define WAVE_LEN 270
static int16_t wavePoints[WAVE_LEN];
static float   wavePhase  = 0.0f;
static float   waveSpeed  = 0.12f;
static bool    wavePaused = false;

// FPS tracking
static uint32_t frameCount  = 0;
static uint32_t lastFpsCalc = 0;
static float    currentFps  = 0.0f;

// RGB LED pins & state
static int8_t pinRed   = -1;
static int8_t pinGreen = -1;
static int8_t pinBlue  = -1;
static int    activeRgbColor = -1; // 0:Red, 1:Green, 2:Blue, 3:Cyan, 4:Yellow, 5:Magenta, 6:White, 7:Off

// Gauge state
static float gaugeTarget = 50.0f;
static float gaugeValue  = 50.0f;

// ── 4. RGB LED HELPER ─────────────────────────────────────────────────────────
void setRgbLed(bool r, bool g, bool b) {
  if (pinRed >= 0)   digitalWrite(pinRed,   r ? LOW : HIGH); // Active LOW
  if (pinGreen >= 0) digitalWrite(pinGreen, g ? LOW : HIGH);
  if (pinBlue >= 0)  digitalWrite(pinBlue,  b ? LOW : HIGH);
}

void applyRgbColor(int idx) {
  activeRgbColor = idx;
  switch (idx) {
    case 0: setRgbLed(true,  false, false); break; // Red
    case 1: setRgbLed(false, true,  false); break; // Green
    case 2: setRgbLed(false, false, true);  break; // Blue
    case 3: setRgbLed(false, true,  true);  break; // Cyan
    case 4: setRgbLed(true,  true,  false); break; // Yellow
    case 5: setRgbLed(true,  false, true);  break; // Magenta
    case 6: setRgbLed(true,  true,  true);  break; // White
    case 7: default: setRgbLed(false, false, false); break; // Off
  }
}

// ── 5. SPLASH SCREEN (RESPONSIVE) ─────────────────────────────────────────────
void showSplashScreen() {
  tft.fillScreen(C_DARK_BG);
  tft.setSwapBytes(true);

  // Center logo horizontally and vertically
  int logoX = (SCRW - LOGO_TFT_WIDTH) / 2;
  int logoY = constrain((SCRH - LOGO_TFT_HEIGHT - 65) / 2, 12, 60);
  tft.pushImage(logoX, logoY, LOGO_TFT_WIDTH, LOGO_TFT_HEIGHT, logo_rgb565);
  tft.drawRoundRect(logoX - 2, logoY - 2, LOGO_TFT_WIDTH + 4, LOGO_TFT_HEIGHT + 4, 6, C_CYAN_ACC);

  int textY = logoY + LOGO_TFT_HEIGHT + 14;
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_TEXT_WHITE, C_DARK_BG);
  tft.drawString("AMK_CYD_Universal_TFT", SCRW / 2, textY, 1);

  char buf[80];
  snprintf(buf, sizeof(buf), "%s | %s (%dx%d)", display.modelName(), display.panelName(), SCRW, SCRH);
  tft.setTextColor(C_GREEN_ACC, C_DARK_BG);
  tft.drawString(buf, SCRW / 2, textY + 14, 1);

  // Animated progress bar (tap screen to skip)
  int barW = constrain(SCRW - 100, 180, 260);
  int barH = 6;
  int barX = (SCRW - barW) / 2;
  int barY = textY + 30;
  tft.drawRoundRect(barX - 2, barY - 2, barW + 4, barH + 4, 3, C_BORDER);

  uint32_t startMs = millis();
  while (millis() - startMs < 1600) {
    float progress = (float)(millis() - startMs) / 1600.0f;
    int fillW = (int)(progress * barW);
    tft.fillRoundRect(barX, barY, fillW, barH, 2, C_CYAN_ACC);

    uint16_t tx, ty;
    if (tft.getTouch(&tx, &ty)) break; // Skip on tap
    delay(15);
  }
}

// ── 6. UI HEADER & TAB MANAGEMENT ─────────────────────────────────────────────
const char* TAB_NAMES[] = { "METER", "WAVE", "RGB / LDR", "LOGO" };

void drawTabBar() {
  int tabW = SCRW / 4;
  for (int i = 0; i < 4; i++) {
    int x = i * tabW;
    uint16_t bg = (i == currentTab) ? C_TAB_SEL : C_TAB_UNSEL;
    uint16_t border = (i == currentTab) ? C_CYAN_ACC : C_BORDER;
    uint16_t textCol = (i == currentTab) ? C_TEXT_WHITE : C_TEXT_MUTED;

    tft.fillRect(x, 0, tabW, 26, bg);
    tft.drawRect(x, 0, tabW, 26, border);
    if (i == currentTab) {
      tft.drawFastHLine(x, 25, tabW, C_CYAN_ACC);
    }

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(textCol, bg);
    tft.drawString(TAB_NAMES[i], x + tabW / 2, 13, 1);
  }
}

void switchTab(int newTab) {
  currentTab = newTab;
  drawTabBar();

  // Clear tab client area
  tft.fillRect(0, 27, SCRW, BOTY - 27, C_DARK_BG);

  if (currentTab == 0) {
    // ── Tab 0: Meter & Slider Layout ──
    int rightX = 142;
    int rightW = SCRW - rightX - 10;

    // Slider Card
    tft.fillRoundRect(rightX, 32, rightW, 64, 4, C_CARD_BG);
    tft.drawRoundRect(rightX, 32, rightW, 64, 4, C_BORDER);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(C_CYAN_ACC, C_CARD_BG);
    tft.drawString("BACKLIGHT DRAG", rightX + 10, 38, 1);

    // Telemetry Card
    int telemH = BOTY - 104;
    tft.fillRoundRect(rightX, 102, rightW, telemH, 4, C_CARD_BG);
    tft.drawRoundRect(rightX, 102, rightW, telemH, 4, C_BORDER);
    tft.setTextColor(C_CYAN_ACC, C_CARD_BG);
    tft.drawString("SYSTEM TELEMETRY", rightX + 10, 108, 1);
    tft.drawFastHLine(rightX + 10, 120, rightW - 20, C_BORDER);
  }
  else if (currentTab == 1) {
    // ── Tab 1: Waveform Layout ──
    int cardW = SCRW - 20;
    int cardH = BOTY - 76;
    tft.fillRoundRect(10, 32, cardW, cardH, 4, C_CARD_BG);
    tft.drawRoundRect(10, 32, cardW, cardH, 4, C_BORDER);

    // Wave speed buttons
    int btnW = (SCRW - 48) / 3;
    int btnH = 32;
    int btnY = BOTY - 38;

    auto drawBtn = [](int x, int y, int w, int h, const char* label) {
      tft.fillRoundRect(x, y, w, h, 4, C_CARD_BG);
      tft.drawRoundRect(x, y, w, h, 4, C_BORDER);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(C_TEXT_WHITE, C_CARD_BG);
      tft.drawString(label, x + w / 2, y + h / 2, 1);
    };

    drawBtn(16, btnY, btnW, btnH, "SLOW");
    drawBtn(16 + btnW + 8, btnY, btnW, btnH, "FAST");
    drawBtn(16 + 2 * (btnW + 8), btnY, btnW, btnH, wavePaused ? "RUN" : "PAUSE");
  }
  else if (currentTab == 2) {
    // ── Tab 2: RGB & LDR Layout ──
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(C_CYAN_ACC, C_DARK_BG);
    tft.drawString("ONBOARD RGB LED (TOUCH TO TEST)", 12, 32, 1);

    // RGB buttons (2 rows of 4)
    const char* rgbNames[] = { "RED", "GREEN", "BLUE", "CYAN", "YELLOW", "MAGENTA", "WHITE", "OFF" };
    uint16_t rgbColors[] = { C_RED_ACC, C_GREEN_ACC, 0x001F, C_CYAN_ACC, C_YELLOW_ACC, 0xF81F, C_TEXT_WHITE, 0x39E7 };

    int bW = (SCRW - 36) / 4;
    int bH = 26;

    for (int i = 0; i < 8; i++) {
      int row = i / 4;
      int col = i % 4;
      int bx = 12 + col * (bW + 4);
      int by = 48 + row * 32;
      tft.fillRoundRect(bx, by, bW, bH, 4, C_CARD_BG);
      tft.drawRoundRect(bx, by, bW, bH, 4, rgbColors[i]);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(rgbColors[i], C_CARD_BG);
      tft.drawString(rgbNames[i], bx + bW / 2, by + bH / 2, 1);
    }

    // LDR Card
    int ldrY = 118;
    int ldrH = 46;
    tft.fillRoundRect(12, ldrY, SCRW - 24, ldrH, 4, C_CARD_BG);
    tft.drawRoundRect(12, ldrY, SCRW - 24, ldrH, 4, C_BORDER);

    // Touch readout box
    int diagY = ldrY + ldrH + 6;
    int diagH = BOTY - diagY - 4;
    tft.fillRoundRect(12, diagY, SCRW - 24, diagH, 4, C_CARD_BG);
    tft.drawRoundRect(12, diagY, SCRW - 24, diagH, 4, C_BORDER);
  }
  else if (currentTab == 3) {
    // ── Tab 3: Logo & Specs Layout ──
    int logoY = constrain(27 + (BOTY - 27 - LOGO_TFT_HEIGHT) / 2, 34, 80);
    tft.setSwapBytes(true);
    tft.pushImage(14, logoY, LOGO_TFT_WIDTH, LOGO_TFT_HEIGHT, logo_rgb565);
    tft.drawRoundRect(12, logoY - 2, LOGO_TFT_WIDTH + 4, LOGO_TFT_HEIGHT + 4, 4, C_CYAN_ACC);

    int cardX = 14 + LOGO_TFT_WIDTH + 14;
    int cardW = SCRW - cardX - 10;
    int cardY = logoY - 2;
    int cardH = LOGO_TFT_HEIGHT + 4;

    tft.fillRoundRect(cardX, cardY, cardW, cardH, 4, C_CARD_BG);
    tft.drawRoundRect(cardX, cardY, cardW, cardH, 4, C_BORDER);

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(C_CYAN_ACC, C_CARD_BG);
    tft.drawString("AMK STUDIO", cardX + 10, cardY + 10, 1);
    tft.setTextColor(C_TEXT_WHITE, C_CARD_BG);
    tft.drawString("CYD Universal TFT", cardX + 10, cardY + 24, 1);
    tft.drawFastHLine(cardX + 10, cardY + 38, cardW - 20, C_BORDER);

    tft.setTextColor(C_GREEN_ACC, C_CARD_BG);
    tft.drawString(String("Model: ") + display.modelName(), cardX + 10, cardY + 46, 1);
    tft.drawString(String("Panel: ") + display.panelName(), cardX + 10, cardY + 62, 1);
    tft.drawString(String("Res  : ") + String(SCRW) + "x" + String(SCRH), cardX + 10, cardY + 78, 1);

    tft.setTextColor(C_TEXT_MUTED, C_CARD_BG);
    tft.drawString("Touch: XPT2046", cardX + 10, cardY + 94, 1);
    tft.drawString("SPI  : 40MHz DMA", cardX + 10, cardY + 110, 1);
    tft.drawString(String("RAM  : ") + String(ESP.getFreeHeap() / 1024) + " KB Free", cardX + 10, cardY + 126, 1);
  }
}

// ── 7. TAB RENDERING ROUTINES ─────────────────────────────────────────────────

// Draw circular gauge into 124x124 sprite and push to screen
void updateGauge(float val) {
  if (!gaugeSprite.created()) return;

  gaugeSprite.fillSprite(C_DARK_BG);

  int cx = 62, cy = 62, r = 54;
  gaugeSprite.drawCircle(cx, cy, r + 4, C_BORDER);
  gaugeSprite.fillCircle(cx, cy, r, C_CARD_BG);
  gaugeSprite.drawCircle(cx, cy, r, C_BORDER);

  // Ticks around arc (135 deg to 405 deg)
  for (int p = 0; p <= 100; p += 10) {
    float rad = (135.0f + p * 2.7f) * 0.0174532925f;
    int x1 = cx + (int)(cosf(rad) * (r - 6));
    int y1 = cy + (int)(sinf(rad) * (r - 6));
    int x2 = cx + (int)(cosf(rad) * r);
    int y2 = cy + (int)(sinf(rad) * r);

    uint16_t tickCol = (p <= 60) ? C_GREEN_ACC : (p <= 80 ? C_YELLOW_ACC : C_RED_ACC);
    gaugeSprite.drawLine(x1, y1, x2, y2, tickCol);
  }

  // Draw animated needle
  float valClamped = constrain(val, 0.0f, 100.0f);
  float nRad = (135.0f + valClamped * 2.7f) * 0.0174532925f;
  int tipX = cx + (int)(cosf(nRad) * (r - 12));
  int tipY = cy + (int)(sinf(nRad) * (r - 12));

  gaugeSprite.drawLine(cx, cy, tipX, tipY, C_RED_ACC);
  gaugeSprite.drawLine(cx + 1, cy, tipX, tipY, C_RED_ACC);
  gaugeSprite.drawLine(cx, cy + 1, tipX, tipY, C_RED_ACC);
  gaugeSprite.fillCircle(cx, cy, 5, C_TEXT_WHITE);
  gaugeSprite.fillCircle(cx, cy, 2, C_RED_ACC);

  // Readout text
  gaugeSprite.setTextDatum(MC_DATUM);
  gaugeSprite.setTextColor(C_CYAN_ACC, C_CARD_BG);
  gaugeSprite.drawString(String(valClamped, 1) + "%", cx, cy + 24, 1);
  gaugeSprite.setTextColor(C_TEXT_MUTED, C_CARD_BG);
  gaugeSprite.drawString("POWER", cx, cy - 22, 1);

  // Push sprite to display
  int gaugeY = 32 + (BOTY - 32 - 124) / 4;
  gaugeSprite.pushSprite(10, gaugeY);
}

// Draw draggable slider bar
void updateSlider() {
  int rightX = 142;
  int rightW = SCRW - rightX - 10;
  int trackX = rightX + 12;
  int trackY = 58;
  int trackW = rightW - 24;
  int trackH = 8;
  int knobX = trackX + ((backlightPct - 10) * trackW) / 90;

  // Track progress
  tft.fillRect(trackX, trackY, knobX - trackX, trackH, C_GREEN_ACC);
  tft.fillRect(knobX, trackY, (trackX + trackW) - knobX, trackH, C_BORDER);

  // Knob
  tft.fillRoundRect(knobX - 6, trackY - 6, 12, 20, 3, C_TEXT_WHITE);
  tft.drawRoundRect(knobX - 6, trackY - 6, 12, 20, 3, C_CYAN_ACC);

  // Readout
  tft.fillRect(rightX + rightW - 60, 36, 50, 14, C_CARD_BG);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(C_YELLOW_ACC, C_CARD_BG);
  tft.drawString(String(backlightPct) + "%", rightX + rightW - 10, 38, 1);
}

// Update telemetry texts on Tab 0
void updateTelemetry() {
  int rightX = 142;
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_TEXT_WHITE, C_CARD_BG);

  char buf[40];
  snprintf(buf, sizeof(buf), "Board : %s", display.modelName());
  tft.drawString(buf, rightX + 10, 128, 1);

  snprintf(buf, sizeof(buf), "Panel : %s", display.panelName());
  tft.drawString(buf, rightX + 10, 144, 1);

  snprintf(buf, sizeof(buf), "FPS   : %.1f", currentFps);
  tft.drawString(buf, rightX + 10, 160, 1);

  snprintf(buf, sizeof(buf), "Free  : %u KB", (unsigned int)(ESP.getFreeHeap() / 1024));
  tft.drawString(buf, rightX + 10, 176, 1);

  uint32_t s = millis() / 1000;
  snprintf(buf, sizeof(buf), "Uptime: %02u:%02u:%02u", (unsigned int)(s / 3600), (unsigned int)((s % 3600) / 60), (unsigned int)(s % 60));
  tft.drawString(buf, rightX + 10, 192, 1);
}

// Draw oscilloscope waveform into 270x110 sprite and push to screen
void updateWaveform() {
  if (!waveSprite.created()) return;

  if (!wavePaused) {
    memmove(&wavePoints[0], &wavePoints[1], sizeof(int16_t) * (WAVE_LEN - 1));
    wavePhase += waveSpeed;
    wavePoints[WAVE_LEN - 1] = (int16_t)(55.0f + sinf(wavePhase) * 36.0f + sinf(wavePhase * 2.3f) * 12.0f);
  }

  waveSprite.fillSprite(C_DARK_BG);

  // Oscilloscope gridlines
  for (int y = 15; y < 110; y += 20) {
    for (int x = 0; x < WAVE_LEN; x += 6) {
      waveSprite.drawPixel(x, y, (y == 55) ? C_BORDER : 0x18E3);
    }
  }
  for (int x = 20; x < WAVE_LEN; x += 30) {
    for (int y = 0; y < 110; y += 6) {
      waveSprite.drawPixel(x, y, 0x18E3);
    }
  }

  // Draw waveform line
  for (int x = 0; x < WAVE_LEN - 1; x++) {
    waveSprite.drawLine(x, wavePoints[x], x + 1, wavePoints[x + 1], C_CYAN_ACC);
  }

  // Leading edge indicator
  waveSprite.fillCircle(WAVE_LEN - 1, wavePoints[WAVE_LEN - 1], 3, C_YELLOW_ACC);

  int waveX = (SCRW - WAVE_LEN) / 2;
  int cardH = BOTY - 76;
  int waveY = 32 + (cardH - 110) / 2;
  waveSprite.pushSprite(waveX, waveY);
}

// Update LDR sensor reading on Tab 2
void updateLdrDisplay() {
  uint32_t ldrMv = analogReadMilliVolts(34);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_TEXT_WHITE, C_CARD_BG);
  tft.drawString("LDR Light Sensor (GPIO 34):", 18, 124, 1);

  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(C_YELLOW_ACC, C_CARD_BG);
  tft.drawString(String(ldrMv) + " mV", SCRW - 20, 124, 1);

  // Progress Bar
  int barW = SCRW - 44;
  int barH = 10;
  int barX = 20;
  int barY = 142;
  int fillW = constrain((int)(ldrMv * barW / 3300), 0, barW);

  uint16_t barCol = (ldrMv < 1200) ? C_GREEN_ACC : (ldrMv < 2400 ? C_YELLOW_ACC : 0xFD20);
  tft.fillRect(barX, barY, fillW, barH, barCol);
  tft.fillRect(barX + fillW, barY, barW - fillW, barH, C_BORDER);
}

// Bottom status line
void updateBottomStatus(bool isTouched, uint16_t tx, uint16_t ty) {
  tft.fillRect(0, BOTY, SCRW, 18, C_HEADER_BG);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(C_TEXT_WHITE, C_HEADER_BG);

  char buf[80];
  if (isTouched) {
    snprintf(buf, sizeof(buf), " Touch: (%3u, %3u) | Pressing", tx, ty);
  } else {
    snprintf(buf, sizeof(buf), " Touch: IDLE | Ready");
  }
  tft.drawString(buf, 8, BOTY + 9, 1);

  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(C_CYAN_ACC, C_HEADER_BG);
  snprintf(buf, sizeof(buf), "Heap: %u KB ", (unsigned int)(ESP.getFreeHeap() / 1024));
  tft.drawString(buf, SCRW - 4, BOTY + 9, 1);
}

// ── 8. ARDUINO SETUP ──────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n[AMK_CYD_Universal_TFT] Starting Universal SpriteDemo...");

  // Initialize display
  tft.init();
  tft.setRotation(1); // Landscape
  SCRW = tft.width();
  SCRH = tft.height();
  BOTY = SCRH - 18;
  tft.setBacklight(backlightPct);

  // Initialize touch
  touch.begin(display);
  touch.setRotation(1);
  touch.loadCalibration();
  touch.loadFilters();

  // Initialize RGB LED pins
  pinRed   = display.redLedPin();
  pinGreen = display.greenLedPin();
  pinBlue  = display.blueLedPin();
  if (pinRed >= 0)   { pinMode(pinRed,   OUTPUT); digitalWrite(pinRed,   HIGH); }
  if (pinGreen >= 0) { pinMode(pinGreen, OUTPUT); digitalWrite(pinGreen, HIGH); }
  if (pinBlue >= 0)  { pinMode(pinBlue,  OUTPUT); digitalWrite(pinBlue,  HIGH); }

  // Allocate Sprites in SRAM
  gaugeSprite.createSprite(124, 124);
  waveSprite.createSprite(270, 110);

  // Pre-seed wave array
  for (int i = 0; i < WAVE_LEN; i++) {
    wavePoints[i] = 55;
  }

  // Show AMK logo splash screen
  showSplashScreen();

  // Load Initial Tab
  switchTab(0);
  updateSlider();
}

// ── 9. ARDUINO LOOP ───────────────────────────────────────────────────────────
void loop() {
  // 1. Calculate FPS
  frameCount++;
  if (millis() - lastFpsCalc >= 1000) {
    currentFps = (float)frameCount * 1000.0f / (float)(millis() - lastFpsCalc);
    frameCount = 0;
    lastFpsCalc = millis();
  }

  // 2. Read Touch Input
  uint16_t tx = 0, ty = 0;
  bool isTouched = tft.getTouch(&tx, &ty);

  // Handle Tab Navigation Bar
  if (isTouched && ty < 26) {
    int touchedTab = tx / (SCRW / 4);
    if (touchedTab >= 0 && touchedTab < 4 && touchedTab != currentTab) {
      switchTab(touchedTab);
      delay(150); // debounce
    }
  }

  // Handle Tab-specific Touch Interaction
  if (currentTab == 0) {
    int rightX = 142;
    int rightW = SCRW - rightX - 10;
    int trackX = rightX + 12;
    int trackW = rightW - 24;

    // Slider Dragging
    if (isTouched && tx >= rightX && tx <= rightX + rightW && ty >= 32 && ty <= 96) {
      backlightPct = constrain(map(tx, trackX, trackX + trackW, 10, 100), 10, 100);
      tft.setBacklight(backlightPct);
      updateSlider();
    }

    // Dynamic Meter Update
    gaugeTarget = 50.0f + sinf(millis() * 0.002f) * 44.0f;
    gaugeValue += (gaugeTarget - gaugeValue) * 0.12f;
    updateGauge(gaugeValue);

    static uint32_t lastTelem = 0;
    if (millis() - lastTelem >= 250) {
      lastTelem = millis();
      updateTelemetry();
    }
  }
  else if (currentTab == 1) {
    int btnW = (SCRW - 48) / 3;
    int btnH = 32;
    int btnY = BOTY - 38;
    int btn1X = 16;
    int btn2X = 16 + btnW + 8;
    int btn3X = 16 + 2 * (btnW + 8);

    // Waveform Speed Buttons
    if (isTouched && ty >= btnY && ty <= btnY + btnH) {
      if (tx >= btn1X && tx <= btn1X + btnW) {
        waveSpeed = 0.05f; // SLOW
      } else if (tx >= btn2X && tx <= btn2X + btnW) {
        waveSpeed = 0.25f; // FAST
      } else if (tx >= btn3X && tx <= btn3X + btnW) {
        wavePaused = !wavePaused; // PAUSE/RUN
        // Redraw button text
        tft.fillRoundRect(btn3X, btnY, btnW, btnH, 4, C_CARD_BG);
        tft.drawRoundRect(btn3X, btnY, btnW, btnH, 4, C_BORDER);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(C_TEXT_WHITE, C_CARD_BG);
        tft.drawString(wavePaused ? "RUN" : "PAUSE", btn3X + btnW / 2, btnY + btnH / 2, 1);
        delay(150);
      }
    }
    updateWaveform();
  }
  else if (currentTab == 2) {
    int bW = (SCRW - 36) / 4;
    int bH = 26;

    // RGB Button Taps
    if (isTouched && ty >= 48 && ty <= 112) {
      for (int i = 0; i < 8; i++) {
        int row = i / 4;
        int col = i % 4;
        int bx = 12 + col * (bW + 4);
        int by = 48 + row * 32;
        if (tx >= bx && tx <= bx + bW && ty >= by && ty <= by + bH) {
          applyRgbColor(i);
          delay(120);
          break;
        }
      }
    }

    // Touch diagnostic box update
    int ldrY = 118, ldrH = 46;
    int diagY = ldrY + ldrH + 6;

    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(C_TEXT_WHITE, C_CARD_BG);
    char buf[60];
    snprintf(buf, sizeof(buf), "Touch State : %s", isTouched ? "TOUCH DETECTED" : "IDLE");
    tft.drawString(buf, 18, diagY + 8, 1);
    if (isTouched) {
      snprintf(buf, sizeof(buf), "Coordinates : (%3u, %3u)", tx, ty);
    } else {
      snprintf(buf, sizeof(buf), "Coordinates : ( ---,  ---)");
    }
    tft.drawString(buf, 18, diagY + 22, 1);

    static uint32_t lastLdr = 0;
    if (millis() - lastLdr >= 200) {
      lastLdr = millis();
      updateLdrDisplay();
    }
  }

  // 3. Update Status Bar
  static uint32_t lastBar = 0;
  if (millis() - lastBar >= 100) {
    lastBar = millis();
    updateBottomStatus(isTouched, tx, ty);
  }

  delay(5);
}
