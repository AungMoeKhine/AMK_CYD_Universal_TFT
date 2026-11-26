/*
 * FreeFontDemo.ino - AMK_CYD_Universal_TFT
 * Comprehensive showcase for Adafruit GFX FreeFonts on ESP32 CYD boards.
 *
 * Demonstrates:
 * 1. Font Families (FreeSans, FreeSerif, FreeMono, TomThumb, Classic GLCD)
 * 2. Proportional Point Sizes (9pt, 12pt, 18pt, 24pt) & Weights (Regular, Bold, Oblique)
 * 3. Giant Vector Scaling Pro-Tip (setTextSize(2) on 24pt for 48pt ~68px numbers)
 * 4. Precise Text Alignment (setTextDatum: Top, Center, Baseline, Bottom)
 * 5. Smooth Off-Screen Sprite Rendering (TFT_eSprite with dynamic live telemetry)
 *
 * Touch or wait 7 seconds to cycle through the interactive showcase pages!
 */

#include <Arduino.h>
#include <AMK_CYD_Universal_TFT.h>

// ── Hardware Driver Objects ───────────────────────────────────────────────────
AMK_display display;
AMK_touch   touch;
TFT_eSPI    tft;
TFT_eSprite telemetrySpr(&tft);

// ── Dynamic Screen Resolution ─────────────────────────────────────────────────
static int SCRW = 320;
static int SCRH = 240;

// ── Color Theme (Deep Dark OLED Style) ────────────────────────────────────────
const uint16_t C_BG        = 0x0000; // Pitch Black
const uint16_t C_PANEL     = 0x10A2; // Slate Header/Footer Bar
const uint16_t C_CARD      = 0x18E3; // Charcoal Card Background
const uint16_t C_BORDER    = 0x2965; // High-Tech Border Outline
const uint16_t C_ACCENT    = 0x07FF; // Electric Cyan
const uint16_t C_WHITE     = 0xFFFF; // Pure White
const uint16_t C_GRAY      = 0x94B2; // Cool Gray
const uint16_t C_GREEN     = 0x07E0; // Vibrant Green
const uint16_t C_YELLOW    = 0xFFE0; // Vibrant Yellow
const uint16_t C_ORANGE    = 0xFDA0; // Warm Amber
const uint16_t C_RED       = 0xF800; // Vivid Red
const uint16_t C_MAGENTA   = 0xF81F; // Magenta

// ── Page & Navigation State ───────────────────────────────────────────────────
const int TOTAL_PAGES = 5;
int currentPage = 0;
uint32_t lastPageChangeTime = 0;
const uint32_t AUTO_CYCLE_MS = 7000; // 7s auto-advance

// Debounce state
bool wasTouched = false;
uint32_t lastTouchRelease = 0;

// Dynamic animation state for Page 4 (Sprite)
float animPhase = 0.0f;
uint32_t lastSpriteUpdate = 0;

// ── Function Prototypes ───────────────────────────────────────────────────────
void drawHeader();
void drawFooter();
void renderCurrentPage();
void drawPage0_Families();
void drawPage1_Sizes();
void drawPage2_Scaling();
void drawPage3_Datums();
void drawPage4_Sprite();
void updateSpriteAnimation();

// ── Page 0: Font Families Overview ────────────────────────────────────────────
void drawPage0_Families() {
  tft.fillRect(0, 27, SCRW, SCRH - 54, C_BG);

  int cardW = SCRW - 20;
  int cardX = 10;

  // Subtitle
  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextColor(C_GRAY, C_BG);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("48 STANDARD GFX VECTOR FONTS BUNDLED OUT OF THE BOX", SCRW / 2, 32);

  // 1. FreeSans (Modern UI)
  tft.drawRoundRect(cardX, 46, cardW, 30, 4, C_BORDER);
  tft.setFreeFont(FSS12);
  tft.setTextColor(C_ACCENT, C_BG);
  tft.setTextDatum(ML_DATUM);
  tft.drawString("FreeSans 12pt", cardX + 10, 61);
  tft.setFreeFont(NULL);
  tft.setTextColor(C_WHITE, C_BG);
  tft.drawString("Modern UI Cards & Clean Telemetry", cardX + 126, 61);

  // 2. FreeSerif (Classic Elegance)
  tft.drawRoundRect(cardX, 80, cardW, 30, 4, C_BORDER);
  tft.setFreeFont(FS12);
  tft.setTextColor(C_YELLOW, C_BG);
  tft.setTextDatum(ML_DATUM);
  tft.drawString("FreeSerif 12pt", cardX + 10, 95);
  tft.setFreeFont(NULL);
  tft.setTextColor(C_WHITE, C_BG);
  tft.drawString("Elegant Decorative Headings", cardX + 126, 95);

  // 3. FreeMono (Terminal & Code)
  tft.drawRoundRect(cardX, 114, cardW, 30, 4, C_BORDER);
  tft.setFreeFont(FM12);
  tft.setTextColor(C_GREEN, C_BG);
  tft.setTextDatum(ML_DATUM);
  tft.drawString("FreeMono 12pt", cardX + 10, 129);
  tft.setFreeFont(NULL);
  tft.setTextColor(C_WHITE, C_BG);
  tft.drawString("Fixed-Width Consoles [0x7F]", cardX + 126, 129);

  // 4. TomThumb (Micro) & Classic GLCD 5x7
  tft.drawRoundRect(cardX, 148, cardW, 30, 4, C_BORDER);
  tft.setFreeFont(TT1);
  tft.setTextColor(C_ORANGE, C_BG);
  tft.setTextDatum(ML_DATUM);
  tft.drawString("TomThumb 5px", cardX + 10, 163);
  tft.setFreeFont(NULL);
  tft.setTextColor(C_GRAY, C_BG);
  tft.drawString("Micro telemetry logs (6px total height)", cardX + 90, 163);

  // 5. Classic GLCD Fixed font
  tft.drawRoundRect(cardX, 182, cardW, 26, 4, C_BORDER);
  tft.setFreeFont(NULL);
  tft.setTextColor(C_WHITE, C_BG);
  tft.setTextDatum(ML_DATUM);
  tft.drawString("Classic 5x7 Font: Selected via tft.setFreeFont(NULL);", cardX + 10, 195);
}

// ── Page 1: Sizes & Weights ───────────────────────────────────────────────────
void drawPage1_Sizes() {
  tft.fillRect(0, 27, SCRW, SCRH - 54, C_BG);

  // Header explanation
  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextColor(C_GRAY, C_BG);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("PROPORTIONAL POINT SIZES & TYPOGRAPHIC WEIGHTS", SCRW / 2, 31);

  // 9pt Regular
  tft.setFreeFont(FSS9);
  tft.setTextColor(C_WHITE, C_BG);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("FreeSans 9pt (Regular) - Small status text", 14, 46);

  // 12pt Bold
  tft.setFreeFont(FSSB12);
  tft.setTextColor(C_ACCENT, C_BG);
  tft.drawString("FreeSans Bold 12pt - Card Headings", 14, 70);

  // 18pt Oblique (Italic)
  tft.setFreeFont(FSSO18);
  tft.setTextColor(C_YELLOW, C_BG);
  tft.drawString("FreeSans Oblique 18pt", 14, 102);

  // 24pt Bold
  tft.setFreeFont(FSSB24);
  tft.setTextColor(C_GREEN, C_BG);
  tft.drawString("FreeSans Bold 24pt", 14, 142);

  // Footer note
  tft.setFreeFont(NULL);
  tft.setTextColor(C_GRAY, C_BG);
  tft.setTextDatum(BC_DATUM);
  tft.drawString("Zero Flash Bloat: Linker strips any font not called in code!", SCRW / 2, SCRH - 34);
}

// ── Page 2: Vector Scaling Pro-Tip ────────────────────────────────────────────
void drawPage2_Scaling() {
  tft.fillRect(0, 27, SCRW, SCRH - 54, C_BG);

  int halfW = (SCRW - 30) / 2;
  int c1X = 10;
  int c2X = c1X + halfW + 10;
  int mid1 = c1X + halfW / 2;
  int mid2 = c2X + halfW / 2;

  // Card 1: 1x Scale (24pt)
  tft.fillRoundRect(c1X, 32, halfW, 110, 4, C_CARD);
  tft.drawRoundRect(c1X, 32, halfW, 110, 4, C_BORDER);
  tft.setFreeFont(NULL);
  tft.setTextColor(C_GRAY, C_CARD);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("STANDARD 1x SCALE", mid1, 38);
  tft.drawString("tft.setTextSize(1);", mid1, 50);

  tft.setFreeFont(FSS24);
  tft.setTextColor(C_WHITE, C_CARD);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("24pt", mid1, 88);
  tft.setFreeFont(NULL);
  tft.setTextColor(C_ACCENT, C_CARD);
  tft.drawString("~34px Height", mid1, 124);

  // Card 2: 2x Scale (Giant 48pt)
  tft.fillRoundRect(c2X, 32, halfW, 110, 4, C_CARD);
  tft.drawRoundRect(c2X, 32, halfW, 110, 4, C_ACCENT);
  tft.setFreeFont(NULL);
  tft.setTextColor(C_YELLOW, C_CARD);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("SCALED 2x (GIANT!)", mid2, 38);
  tft.drawString("tft.setTextSize(2);", mid2, 50);

  tft.setFreeFont(FSS24);
  tft.setTextSize(2); // Scale FreeFont glyphs!
  tft.setTextColor(C_YELLOW, C_CARD);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("48pt", mid2, 90);
  tft.setTextSize(1); // Restore standard scale

  tft.setFreeFont(NULL);
  tft.setTextColor(C_YELLOW, C_CARD);
  tft.drawString("~68px Tall Vector!", mid2, 124);

  // Pro-Tip Banner at bottom
  tft.fillRoundRect(10, 148, SCRW - 20, 56, 4, C_PANEL);
  tft.drawRoundRect(10, 148, SCRW - 20, 56, 4, C_BORDER);

  tft.setFreeFont(FSSB9);
  tft.setTextColor(C_ACCENT, C_PANEL);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("PRO-TIP: Scaling Vector FreeFonts", 18, 153);

  tft.setFreeFont(NULL);
  tft.setTextColor(C_WHITE, C_PANEL);
  tft.drawString("Call tft.setTextSize(2) with any FreeFont for massive gauges,", 18, 172);
  tft.drawString("digital clocks, and hero metrics with ZERO extra flash memory!", 18, 185);
}

// ── Page 3: Precise Text Alignment (Datums) ───────────────────────────────────
void drawPage3_Datums() {
  tft.fillRect(0, 27, SCRW, SCRH - 54, C_BG);

  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextColor(C_GRAY, C_BG);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("NINE ACCURATE TEXT DATUM ANCHORS (setTextDatum)", SCRW / 2, 31);

  // Draw 4 test boxes with crosshairs
  int boxW = (SCRW - 36) / 2;
  int boxH = (SCRH >= 320) ? 95 : 74;

  struct DatumDemo {
    int col, row;
    uint8_t datum;
    const char* label;
    const char* datumName;
  } demos[4] = {
    { 0, 0, TL_DATUM, "Top-Left",      "TL_DATUM" },
    { 1, 0, TR_DATUM, "Top-Right",     "TR_DATUM" },
    { 0, 1, MC_DATUM, "Middle-Center", "MC_DATUM" },
    { 1, 1, BR_DATUM, "Bottom-Right",  "BR_DATUM" }
  };

  for (int i = 0; i < 4; i++) {
    int bx = 12 + demos[i].col * (boxW + 12);
    int by = 44 + demos[i].row * (boxH + 8);
    tft.drawRoundRect(bx, by, boxW, boxH, 4, C_BORDER);

    // Crosshair target in the box
    int cx = bx + boxW / 2;
    int cy = by + boxH / 2;
    tft.drawFastHLine(cx - 12, cy, 25, 0x3186);
    tft.drawFastVLine(cx, cy - 12, 25, 0x3186);
    tft.fillCircle(cx, cy, 2, C_RED);

    // Render string using datum
    tft.setFreeFont(FSS9);
    tft.setTextColor(C_WHITE, C_BG);
    tft.setTextDatum(demos[i].datum);
    tft.drawString(demos[i].label, cx, cy);

    // Label datum name
    tft.setFreeFont(NULL);
    tft.setTextColor(C_ACCENT, C_BG);
    tft.setTextDatum(BC_DATUM);
    tft.drawString(demos[i].datumName, bx + boxW / 2, by + boxH - 4);
  }

  tft.setFreeFont(NULL);
  tft.setTextColor(C_GRAY, C_BG);
  tft.setTextDatum(BC_DATUM);
  tft.drawString("Crosshair dot indicates the (X, Y) coordinate anchor point.", SCRW / 2, SCRH - 30);
}

// ── Page 4: Smooth Off-Screen Sprite (TFT_eSprite) ─────────────────────────────
void drawPage4_Sprite() {
  tft.fillRect(0, 27, SCRW, SCRH - 54, C_BG);

  // Background frame card
  int frameW = SCRW - 36;
  int frameH = SCRH - 62;
  tft.drawRoundRect(18, 30, frameW, frameH, 6, C_BORDER);

  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextColor(C_GRAY, C_BG);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("SMOOTH OFF-SCREEN SPRITE RENDERING (TFT_eSprite)", SCRW / 2, 34);

  // Initial draw of sprite
  updateSpriteAnimation();
}

void updateSpriteAnimation() {
  if (currentPage != 4) return;

  int sprW = 270;
  int sprH = 136;
  int sprX = (SCRW - sprW) / 2;
  int sprY = (SCRH - sprH) / 2 + 5;

  // Clear sprite buffer (270x136, 16-bit color in RAM)
  telemetrySpr.fillSprite(C_CARD);
  telemetrySpr.drawRoundRect(0, 0, sprW, sprH, 5, C_BORDER);

  // Header inside sprite
  telemetrySpr.setFreeFont(FSSB9);
  telemetrySpr.setTextColor(C_ACCENT, C_CARD);
  telemetrySpr.setTextDatum(TL_DATUM);
  telemetrySpr.drawString("LIVE SYSTEM TELEMETRY", 12, 10);

  // Status icon / dot
  telemetrySpr.fillCircle(sprW - 16, 16, 4, C_GREEN);

  // Calculate dynamic oscillating values
  animPhase += 0.05f;
  if (animPhase > 6.2831f) animPhase -= 6.2831f;
  float val = 50.0f + 45.0f * sinf(animPhase);

  // Big hero metric using FreeSans 24pt
  telemetrySpr.setFreeFont(FSS24);
  telemetrySpr.setTextColor(C_WHITE, C_CARD);
  telemetrySpr.setTextDatum(TL_DATUM);
  char valBuf[16];
  snprintf(valBuf, sizeof(valBuf), "%04.1f", val);
  telemetrySpr.drawString(valBuf, 12, 34);

  // Unit using FreeSans 12pt
  telemetrySpr.setFreeFont(FSS12);
  telemetrySpr.setTextColor(C_YELLOW, C_CARD);
  telemetrySpr.drawString("MHz", 145, 45);

  // Sparkline bar graph inside sprite
  int barX = 12;
  int barY = 76;
  int barW = sprW - 24;
  int barH = 10;
  telemetrySpr.drawRoundRect(barX, barY, barW, barH, 2, C_BORDER);
  int fillW = (int)((val / 100.0f) * (barW - 4));
  if (fillW < 2) fillW = 2;
  telemetrySpr.fillRoundRect(barX + 2, barY + 2, fillW, barH - 4, 1, (val > 80.0f) ? C_RED : C_ACCENT);

  // Monospaced terminal readout inside sprite
  telemetrySpr.setFreeFont(FM9);
  telemetrySpr.setTextColor(C_GREEN, C_CARD);
  telemetrySpr.setTextDatum(TL_DATUM);
  char monoBuf[32];
  snprintf(monoBuf, sizeof(monoBuf), "[HEAP: %u KB]", (unsigned)(ESP.getFreeHeap() / 1024));
  telemetrySpr.drawString(monoBuf, 12, 94);

  // Classic GLCD font inside sprite
  telemetrySpr.setFreeFont(NULL);
  telemetrySpr.setTextColor(C_GRAY, C_CARD);
  telemetrySpr.drawString("Buffer: 270x136 16-bit | 0 Screen Tearing", 12, 118);

  // Push off-screen sprite buffer directly to physical display
  telemetrySpr.pushSprite(sprX, sprY);
}

// ── Persistent Navigation Header & Footer ─────────────────────────────────────
void drawHeader() {
  tft.fillRect(0, 0, SCRW, 26, C_PANEL);
  tft.drawFastHLine(0, 26, SCRW, C_ACCENT);

  tft.setFreeFont(FSS9);
  tft.setTextColor(C_WHITE, C_PANEL);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("AMK FreeFont Showcase", SCRW / 2, 13);
}

void drawFooter() {
  int footY = SCRH - 26;
  tft.fillRect(0, footY, SCRW, 26, C_PANEL);
  tft.drawFastHLine(0, footY, SCRW, C_BORDER);

  // [ < PREV ] Button
  tft.fillRoundRect(6, footY + 3, 72, 20, 3, C_CARD);
  tft.drawRoundRect(6, footY + 3, 72, 20, 3, C_BORDER);
  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextColor(C_ACCENT, C_CARD);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("< PREV", 42, footY + 13);

  // Page Indicator
  char pageBuf[32];
  const char* titles[TOTAL_PAGES] = {
    "1/5: Families",
    "2/5: Sizes",
    "3/5: Pro-Scaling",
    "4/5: Datums",
    "5/5: Sprite RAM"
  };
  snprintf(pageBuf, sizeof(pageBuf), "PAGE %s", titles[currentPage]);
  tft.setTextColor(C_WHITE, C_PANEL);
  tft.drawString(pageBuf, SCRW / 2, footY + 13);

  // [ NEXT > ] Button
  int nextX = SCRW - 78;
  tft.fillRoundRect(nextX, footY + 3, 72, 20, 3, C_CARD);
  tft.drawRoundRect(nextX, footY + 3, 72, 20, 3, C_BORDER);
  tft.setTextColor(C_ACCENT, C_CARD);
  tft.drawString("NEXT >", nextX + 36, footY + 13);
}

void renderCurrentPage() {
  drawHeader();
  drawFooter();

  switch (currentPage) {
    case 0: drawPage0_Families(); break;
    case 1: drawPage1_Sizes();    break;
    case 2: drawPage2_Scaling();  break;
    case 3: drawPage3_Datums();   break;
    case 4: drawPage4_Sprite();   break;
  }
  lastPageChangeTime = millis();
}

void nextPage() {
  currentPage = (currentPage + 1) % TOTAL_PAGES;
  renderCurrentPage();
}

void prevPage() {
  currentPage = (currentPage - 1 + TOTAL_PAGES) % TOTAL_PAGES;
  renderCurrentPage();
}

// ── Setup & Loop ──────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n[AMK_CYD_Universal_TFT] Starting FreeFontDemo...");

  // 1. Universal 1-line auto-setup (v1.6.0)
  tft.autoSetup(3);
  tft.autoCalibrate(); // Auto-calibrates uncalibrated resistive touch (< 1 ms if already saved; 0 ms if capacitive)
  SCRW = tft.width();
  SCRH = tft.height();
  tft.fillScreen(C_BG);

  // 3. Create Off-Screen Sprite Buffer (16-bit color in RAM)
  telemetrySpr.setColorDepth(16);
  telemetrySpr.createSprite(270, 136);

  // 4. Render initial page
  renderCurrentPage();
}

void loop() {
  tft.adjustBrightness(); // Built-in ambient light auto-dimming

  uint16_t tx = 0, ty = 0;
  bool isPressed = touch.read(tx, ty);

  // Handle Touch Navigation
  if (isPressed && !wasTouched && (millis() - lastTouchRelease > 250)) {
    wasTouched = true;

    // Check Footer Buttons (Y >= SCRH - 30)
    if (ty >= (uint16_t)(SCRH - 30)) {
      if (tx < 100) {
        prevPage();
      } else if (tx > (uint16_t)(SCRW - 100)) {
        nextPage();
      }
    } else {
      // Tap anywhere on screen body to advance
      nextPage();
    }
  } else if (!isPressed && wasTouched) {
    wasTouched = false;
    lastTouchRelease = millis();
  }

  // Live Sprite Animation on Page 4
  if (currentPage == 4 && (millis() - lastSpriteUpdate >= 40)) {
    lastSpriteUpdate = millis();
    updateSpriteAnimation();
  }

  // Auto-advance timer (cycles every 7 seconds if screen is untouched)
  if (millis() - lastPageChangeTime >= AUTO_CYCLE_MS) {
    nextPage();
  }

  delay(10);
}
