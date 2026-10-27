/*
 * CYD_TouchCal_Universal.ino - Universal Touch Calibration Tool
 * 
 * Instructions:
 * 1. Upload this sketch to your CYD board (Tools > Board: "ESP32 Dev Module").
 * 2. Select your desired orientation (Landscape or Portrait).
 * 3. Use a stylus to tap and hold the center of each crosshair.
 * 4. Adjust the pressure and jump filter sliders if necessary.
 * 5. Tap "TEST DRAW >>" to test accuracy across the whole screen.
 * 6. Tap "SAVE*" in the top right corner to store calibration permanently in NVS Flash.
 * 7. Open any other library example—your saved calibration will be loaded automatically!
 */

#include <Arduino.h>
#include <AMK_CYD_Universal_TFT.h>
#include <math.h>

// Dynamic board detection based on hardware probing
inline const char* getDetectedBoardName() {
  const char* m = display.modelName();
  if (m && strcmp(m, "unknown") != 0) return m;
  if (display.panel() == CYDR_PANEL_ST7796) return "CYD 3.5\" / 4.0\" (ST7796)";
  if (display.board() == CYDR_BOARD_SHARED_TOUCH) return "CYD 3.2\" (ST7789)";
  return "CYD 2.4\" / 2.8\" (ILI9341/ST7789)";
}

#define BUZZER_PIN        26
#define TOUCH_IRQ_PIN     36
#define CAL_BOOT_PIN      0

// Driver objects
AMK_CYD_display display;
AMK_CYD_touch   touch;
TFT_eSPI        tft;

// ── Custom Theme: High-Tech Deep Dark & Electric Cyan ─────────────────────────
const uint16_t C_BG        = 0x0000; // Pitch Black (True OLED-style deep contrast)
const uint16_t C_PANEL     = 0x10A2; // Dark Slate Header Bar (RGB: 16, 32, 48)
const uint16_t C_CARD      = 0x18E3; // Deep Charcoal Card Fill
const uint16_t C_BORDER    = 0x2965; // High-Tech Border Outline
const uint16_t C_ACCENT    = 0x07FF; // Electric Cyan (Draw tracking line/dots & accent)
const uint16_t C_TEXT      = 0xFFFF; // Crisp Pure White
const uint16_t C_MUTED     = 0x94B2; // Cool Silver Grey
const uint16_t C_BTN_SEL   = 0x0215; // Deep Cobalt Blue for active selection
const uint16_t C_BTN_UNSEL = 0x18E3; // Dark Charcoal for inactive selection
const uint16_t C_BTN_START = 0x04C0; // Vibrant Emerald for Start / Confirm / Saved
const uint16_t C_BTN_RED   = 0xA800; // Vivid Crimson for Clear / Reset
const uint16_t C_BTN_WARN  = 0xFBC0; // Amber Gold for Re-calibrate / Unsaved state

static int TARGET_ROTATION = 3;
static int SCRW = 320;
static int SCRH = 240;
static int NATW = 240;
static int NATH = 320;

static const uint16_t DEF_MINP = 25;
static const uint16_t DEF_JUMP = 120;

static const int   JITTER_RAW  = 130;
static const int   MIN_SAMPLES = 8;
static const float ACCEPT_RMS  = 6.0f;
static const float ACCEPT_MAX  = 16.0f;

static uint16_t base_xMin = 200, base_xMax = 3900;
static uint16_t base_yMin = 200, base_yMax = 3900;

static uint16_t activeMinP = DEF_MINP;
static uint16_t activeJump = DEF_JUMP;
static float    cornerStretchPct = 0.0f;
static bool     hasUnsavedChanges = false;

#define MAXPTS 30
static int   gx[MAXPTS], gy[MAXPTS];
static float rawXs[MAXPTS], rawYs[MAXPTS];
static float natX[MAXPTS],  natY[MAXPTS];

static const int   PSLIDER_MAX  = 1000;
static const float PSLIDER_PMAX = 2000.0f;
static const float PSLIDER_K    = 4.0f;

static void setBacklight(uint8_t brightness) {
  // AMK_TFT auto-detects GPIO21 (own-touch) or GPIO27 (shared-touch)
  display.setBacklight((int)brightness * 100 / 255);
}

static void beep(int ms = 12) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(ms);
  digitalWrite(BUZZER_PIN, LOW);
}

static uint16_t sliderToPressure(int s) {
  s = constrain(s, 0, PSLIDER_MAX);
  float t = (float)s / PSLIDER_MAX;
  float v = (expf(PSLIDER_K * t) - 1.0f) / (expf(PSLIDER_K) - 1.0f);
  return (uint16_t)(v * PSLIDER_PMAX + 0.5f);
}

static int pressureToSlider(uint16_t p) {
  float v = (float)p / PSLIDER_PMAX;
  if (v < 0.0f) v = 0.0f;
  if (v > 1.0f) v = 1.0f;
  float t = logf(1.0f + v * (expf(PSLIDER_K) - 1.0f)) / PSLIDER_K;
  return (int)(t * PSLIDER_MAX + 0.5f);
}

static uint16_t clampRaw(float v) {
  if (v < 0.0f) v = 0.0f;
  if (v > 4095.0f) v = 4095.0f;
  return (uint16_t)(v + 0.5f);
}

static void screenToNative(int sx, int sy, int rot, int nativeW, int nativeH, float &px, float &py) {
  int W = nativeW - 1;
  int H = nativeH - 1;
  uint8_t r = cydr_normalize_rotation(rot);
  switch (r) {
    case 0: px = sx;     py = sy;     break;
    case 1: px = W - sy; py = sx;     break;
    case 2: px = W - sx; py = H - sy; break;
    case 3: px = sy;     py = H - sx; break;
    default: px = sx;    py = sy;     break;
  }
}

static void nativeToScreen(float px, float py, int rot, int nativeW, int nativeH, float &sx, float &sy) {
  int W = nativeW - 1;
  int H = nativeH - 1;
  uint8_t r = cydr_normalize_rotation(rot);
  switch (r) {
    case 0: sx = px;     sy = py;     break;
    case 1: sx = py;     sy = W - px; break;
    case 2: sx = W - px; sy = H - py; break;
    case 3: sx = H - py; sy = px;     break;
    default: sx = px;    sy = py;     break;
  }
}

static void applyStretchedCalibrationRAM(uint16_t &outXMin, uint16_t &outXMax,
                                         uint16_t &outYMin, uint16_t &outYMax) {
  float spanX = (float)base_xMax - (float)base_xMin;
  float spanY = (float)base_yMax - (float)base_yMin;

  float shrinkX = spanX * (cornerStretchPct / 100.0f);
  float shrinkY = spanY * (cornerStretchPct / 100.0f);

  outXMin = clampRaw((float)base_xMin + shrinkX);
  outXMax = clampRaw((float)base_xMax - shrinkX);
  outYMin = clampRaw((float)base_yMin + shrinkY);
  outYMax = clampRaw((float)base_yMax - shrinkY);

  if (outXMin >= outXMax) { outXMin = base_xMin; outXMax = base_xMax; }
  if (outYMin >= outYMax) { outYMin = base_yMin; outYMax = base_yMax; }

  touch.setCalibration(outXMin, outXMax, outYMin, outYMax);
}

static inline bool safeReadRaw(uint16_t &a, uint16_t &b) {
  return touch.readRaw(a, b);
}

static inline bool safeReadTouch(uint16_t &x, uint16_t &y) {
  return touch.read(x, y);
}

static void waitRelease() {
  uint16_t a, b;
  uint32_t lastDown = millis();
  while (millis() - lastDown < 150) {
    if (safeReadRaw(a, b)) lastDown = millis();
    delay(5);
  }
}

static bool captureTap(float &rx, float &ry, bool canSkip, bool &skipped) {
  waitRelease();
  uint16_t a, b;
  while (!safeReadRaw(a, b)) {
    if (canSkip && digitalRead(CAL_BOOT_PIN) == LOW) {
      skipped = true;
      return false;
    }
    delay(5);
  }

  uint32_t sx = 0, sy = 0;
  int n = 0;
  uint16_t minx = 4095, maxx = 0, miny = 4095, maxy = 0;
  uint32_t start = millis();
  while (millis() - start < 300) {
    if (safeReadRaw(a, b)) {
      sx += a; sy += b; n++;
      if (a < minx) minx = a; if (a > maxx) maxx = a;
      if (b < miny) miny = b; if (b > maxy) maxy = b;
    }
    delay(8);
  }
  waitRelease();
  if (n < MIN_SAMPLES) return false;
  if ((maxx - minx) > JITTER_RAW || (maxy - miny) > JITTER_RAW) return false;
  rx = (float)sx / n;
  ry = (float)sy / n;
  return true;
}

static void linfit(const float *I, const float *D, int n, float &a, float &b) {
  float sI = 0, sD = 0, sII = 0, sID = 0;
  for (int i = 0; i < n; i++) {
    sI  += I[i];
    sD  += D[i];
    sII += I[i] * I[i];
    sID += I[i] * D[i];
  }
  float den = n * sII - sI * sI;
  b = (den != 0.0f) ? (n * sID - sI * sD) / den : 0.0f;
  a = (sD - b * sI) / n;
}

static int genGrid(int cols, int rows) {
  const int MX = SCRW / 10;
  const int MY = SCRH / 10;
  int n = 0;
  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
      gx[n] = (cols <= 1) ? SCRW / 2 : MX + c * (SCRW - 1 - 2 * MX) / (cols - 1);
      gy[n] = (rows <= 1) ? SCRH / 2 : MY + r * (SCRH - 1 - 2 * MY) / (rows - 1);
      n++;
    }
  }
  return n;
}

static void drawCrosshair(int cx, int cy, uint16_t lineCol, uint16_t centerCol) {
  tft.drawFastHLine(0, cy - 1, SCRW, lineCol);
  tft.drawFastHLine(0, cy,     SCRW, lineCol);
  tft.drawFastHLine(0, cy + 1, SCRW, lineCol);
  tft.drawFastVLine(cx - 1, 0, SCRH, lineCol);
  tft.drawFastVLine(cx,     0, SCRH, lineCol);
  tft.drawFastVLine(cx + 1, 0, SCRH, lineCol);
  tft.fillRect(cx - 4, cy - 4, 9, 9, centerCol);
}

static float runCalibrationGrid(int cols, int rows,
                                uint16_t &xMin, uint16_t &xMax,
                                uint16_t &yMin, uint16_t &yMax,
                                float &maxErr, bool canSkip, bool &skipped) {
  int n = genGrid(cols, rows);
  skipped = false;

  for (int i = 0; i < n; i++) {
    bool ok = false;
    while (!ok) {
      tft.fillScreen(C_BG);
      tft.setTextDatum(TC_DATUM);
      tft.setTextColor(C_ACCENT, C_BG);
      tft.drawString("TOUCH CALIBRATION (STAGE 1)", SCRW / 2, 6, 1);

      char buf[64];
      snprintf(buf, sizeof(buf), "Point %d/%d - Tap & hold crosshair", i + 1, n);
      tft.setTextColor(C_TEXT, C_BG);
      tft.drawString(buf, SCRW / 2, 18, 1);

      if (canSkip) {
        tft.setTextDatum(BC_DATUM);
        tft.setTextColor(C_MUTED, C_BG);
        tft.drawString("Press BOOT button to skip", SCRW / 2, SCRH - 4, 1);
      }

      drawCrosshair(gx[i], gy[i], C_TEXT, TFT_RED);

      ok = captureTap(rawXs[i], rawYs[i], canSkip, skipped);
      if (skipped) return -1.0f;

      if (!ok) {
        drawCrosshair(gx[i], gy[i], C_BG, C_BG);
        tft.fillRect(0, 16, SCRW, 14, C_BG);
        tft.setTextDatum(TC_DATUM);
        tft.setTextColor(TFT_YELLOW, C_BG);
        tft.drawString("Too shaky! Hold stylus steady...", SCRW / 2, 18, 1);
        beep(40);
        delay(700);
      } else {
        beep(12);
      }
    }
  }

  for (int i = 0; i < n; i++) {
    screenToNative(gx[i], gy[i], TARGET_ROTATION, NATW, NATH, natX[i], natY[i]);
  }

  float ax, bx, ay, by;
  linfit(natX, rawXs, n, ax, bx);
  linfit(natY, rawYs, n, ay, by);

  const bool mx = touch.mirrorsX(), my = touch.mirrorsY();
  uint16_t rx0 = clampRaw(ax), rx1 = clampRaw(ax + bx * (NATW - 1));
  uint16_t ry0 = clampRaw(ay), ry1 = clampRaw(ay + by * (NATH - 1));
  xMin = mx ? rx1 : rx0;  xMax = mx ? rx0 : rx1;
  yMin = my ? ry1 : ry0;  yMax = my ? ry0 : ry1;
  if (xMin > xMax) { uint16_t t = xMin; xMin = xMax; xMax = t; }
  if (yMin > yMax) { uint16_t t = yMin; yMin = yMax; yMax = t; }

  float sumSq = 0;
  maxErr = 0;
  for (int i = 0; i < n; i++) {
    float prePx  = (xMax != xMin) ? (rawXs[i] - xMin) * (NATW - 1.0f) / (xMax - xMin) : 0;
    float prePy  = (yMax != yMin) ? (rawYs[i] - yMin) * (NATH - 1.0f) / (yMax - yMin) : 0;
    float predPx = mx ? (NATW - 1) - prePx : prePx;
    float predPy = my ? (NATH - 1) - prePy : prePy;

    float predSx, predSy;
    nativeToScreen(predPx, predPy, TARGET_ROTATION, NATW, NATH, predSx, predSy);

    float ex = predSx - gx[i];
    float ey = predSy - gy[i];
    float err = sqrtf(ex * ex + ey * ey);
    sumSq += err * err;
    if (err > maxErr) maxErr = err;
  }
  return sqrtf(sumSq / n);
}

static void runStage1Calibration(bool canSkip) {
  setBacklight(255);

  uint16_t xMin = 0, xMax = 0, yMin = 0, yMax = 0;
  float rms = 0, maxErr = 0;
  bool skipped = false;

  const int cols[] = {4, 5, 6};
  const int rows[] = {3, 4, 5};

  for (int t = 0; t < 3; t++) {
    if (t > 0) {
      tft.fillScreen(C_BG);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(TFT_ORANGE, C_BG);
      tft.drawString("Fit error high - adding points...", SCRW / 2, SCRH / 2 - 8, 1);
      char errBuf[64];
      snprintf(errBuf, sizeof(errBuf), "RMS: %.1f px | Max: %.1f px", rms, maxErr);
      tft.drawString(errBuf, SCRW / 2, SCRH / 2 + 8, 1);
      delay(1400);
    }

    rms = runCalibrationGrid(cols[t], rows[t], xMin, xMax, yMin, yMax, maxErr, canSkip, skipped);
    if (skipped) break;
    if (rms <= ACCEPT_RMS && maxErr <= ACCEPT_MAX) break;
  }

  if (skipped) {
    while (digitalRead(CAL_BOOT_PIN) == LOW) delay(10);
  } else {
    base_xMin = xMin; base_xMax = xMax;
    base_yMin = yMin; base_yMax = yMax;
    cornerStretchPct = 0.0f;
    hasUnsavedChanges = true;

    uint16_t sxMin, sxMax, syMin, syMax;
    applyStretchedCalibrationRAM(sxMin, sxMax, syMin, syMax);
  }
}

static void drawButton(int x, int y, int w, int h, const char* label, uint16_t bg, uint16_t fg) {
  tft.fillRoundRect(x, y, w, h, 6, bg);
  tft.drawRoundRect(x, y, w, h, 6, C_ACCENT);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(fg, bg);
  tft.drawString(label, x + w / 2, y + h / 2, 1);
}

static void drawSliderBar(int sliderX, int sliderW, int yCenter, int val, int maxVal, uint16_t fillCol) {
  tft.fillRect(sliderX - 4, yCenter - 8, sliderW + 8, 16, C_PANEL);
  tft.fillRoundRect(sliderX, yCenter - 3, sliderW, 6, 3, 0x2124);
  int knobX = sliderX + (constrain(val, 0, maxVal) * sliderW) / maxVal;
  if (knobX > sliderX) {
    tft.fillRoundRect(sliderX, yCenter - 3, knobX - sliderX, 6, 3, fillCol);
  }
  tft.fillCircle(knobX, yCenter, 7, C_TEXT);
  tft.drawCircle(knobX, yCenter, 7, C_BG);
}

static void runStage2FilterSliders() {
  setBacklight(255);
  touch.setFilters(0, 0);

  int sliP = pressureToSlider(activeMinP);
  int sliJ = constrain((int)activeJump, 0, 200);

  tft.fillScreen(C_BG);

  int sliderX = 42;
  int sliderW = SCRW - 84;

  tft.fillRect(0, 0, SCRW, 22, C_PANEL);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(C_ACCENT, C_PANEL);
  tft.drawString("STAGE 2: FILTERS & CORNERS", 6, 11, 1);

  tft.fillRect(SCRW - 74, 2, 72, 18, 0x1082);

  drawButton(4,         38, 28, 22, "-", C_BTN_UNSEL, C_TEXT);
  drawButton(SCRW - 32, 38, 28, 22, "+", C_BTN_UNSEL, C_TEXT);

  drawButton(4,         84, 28, 22, "-", C_BTN_UNSEL, C_TEXT);
  drawButton(SCRW - 32, 84, 28, 22, "+", C_BTN_UNSEL, C_TEXT);

  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(C_TEXT, C_BG);
  tft.drawString("Corner Stretch:", 6, 140, 1);
  drawButton(SCRW - 74, 130, 28, 22, "-", C_BTN_UNSEL, C_TEXT);
  drawButton(SCRW - 32, 130, 28, 22, "+", C_BTN_UNSEL, C_TEXT);

  int btnHalfW = (SCRW - 16) / 2;
  drawButton(4,           SCRH - 42, btnHalfW, 36, "RE-CAL PTS",   C_BTN_WARN, C_TEXT);
  drawButton(8 + btnHalfW, SCRH - 42, btnHalfW, 36, "TEST DRAW >>", C_BTN_START, C_TEXT);

  auto refreshMinPUI = [&]() {
    activeMinP = sliderToPressure(sliP);
    tft.fillRect(4, 24, SCRW - 8, 13, C_BG);
    char buf[64];
    snprintf(buf, sizeof(buf), "Min Pressure: %u", activeMinP);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_YELLOW, C_BG);
    tft.drawString(buf, SCRW / 2, 25, 1);
    drawSliderBar(sliderX, sliderW, 49, sliP, PSLIDER_MAX, C_ACCENT);
  };

  auto refreshJumpUI = [&]() {
    activeJump = (uint16_t)sliJ;
    tft.fillRect(4, 70, SCRW - 8, 13, C_BG);
    char buf[64];
    if (activeJump == 0) snprintf(buf, sizeof(buf), "Jump Filter: OFF");
    else                 snprintf(buf, sizeof(buf), "Ignore Jumps > %u px", activeJump);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_YELLOW, C_BG);
    tft.drawString(buf, SCRW / 2, 71, 1);
    drawSliderBar(sliderX, sliderW, 95, sliJ, 200, TFT_MAGENTA);
  };

  auto refreshStretchUI = [&]() {
    tft.fillRect(SCRW - 140, 134, 62, 16, C_BG);
    char buf[24];
    snprintf(buf, sizeof(buf), "%+.1f%%", cornerStretchPct);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(C_ACCENT, C_BG);
    tft.drawString(buf, SCRW - 109, 142, 1);

    uint16_t sxMin, sxMax, syMin, syMax;
    applyStretchedCalibrationRAM(sxMin, sxMax, syMin, syMax);
  };

  refreshMinPUI();
  refreshJumpUI();
  refreshStretchUI();

  uint32_t lastPressShowTime = 0;
  uint16_t lastPressVal = 9999;
  uint32_t lastBtnTap = 0;
  bool done = false;

  while (!done) {
    uint16_t tx = 0, ty = 0;
    bool touched = safeReadTouch(tx, ty);
    uint16_t curP = touch.pressure();

    if (millis() - lastPressShowTime > 60) {
      lastPressShowTime = millis();
      if (curP != lastPressVal) {
        lastPressVal = curP;
        tft.fillRect(SCRW - 74, 2, 72, 18, 0x1082);
        char pBuf[24];
        snprintf(pBuf, sizeof(pBuf), "P:%u", curP);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(curP >= activeMinP ? C_ACCENT : TFT_ORANGE, 0x1082);
        tft.drawString(pBuf, SCRW - 38, 11, 1);
      }
    }

    if (touched) {
      if (tx >= sliderX - 6 && tx <= sliderX + sliderW + 6 && ty >= 36 && ty <= 62) {
        sliP = constrain((int)(tx - sliderX) * PSLIDER_MAX / sliderW, 0, PSLIDER_MAX);
        hasUnsavedChanges = true;
        refreshMinPUI();
      } else if (tx >= sliderX - 6 && tx <= sliderX + sliderW + 6 && ty >= 82 && ty <= 108) {
        sliJ = constrain((int)(tx - sliderX) * 200 / sliderW, 0, 200);
        hasUnsavedChanges = true;
        refreshJumpUI();
      } else if (millis() - lastBtnTap > 140) {
        if (tx >= 2 && tx <= 34 && ty >= 36 && ty <= 62) {
          if (activeMinP > 0) activeMinP--;
          sliP = pressureToSlider(activeMinP);
          hasUnsavedChanges = true;
          refreshMinPUI();
          beep(8);
          lastBtnTap = millis();
        } else if (tx >= SCRW - 34 && tx <= SCRW - 2 && ty >= 36 && ty <= 62) {
          if (activeMinP < 2000) activeMinP++;
          sliP = pressureToSlider(activeMinP);
          hasUnsavedChanges = true;
          refreshMinPUI();
          beep(8);
          lastBtnTap = millis();
        } else if (tx >= 2 && tx <= 34 && ty >= 82 && ty <= 108) {
          sliJ = max(0, sliJ - 5);
          hasUnsavedChanges = true;
          refreshJumpUI();
          beep(8);
          lastBtnTap = millis();
        } else if (tx >= SCRW - 34 && tx <= SCRW - 2 && ty >= 82 && ty <= 108) {
          sliJ = min(200, sliJ + 5);
          hasUnsavedChanges = true;
          refreshJumpUI();
          beep(8);
          lastBtnTap = millis();
        } else if (tx >= SCRW - 76 && tx <= SCRW - 44 && ty >= 128 && ty <= 154) {
          cornerStretchPct = max(-15.0f, cornerStretchPct - 0.5f);
          hasUnsavedChanges = true;
          refreshStretchUI();
          beep(8);
          lastBtnTap = millis();
        } else if (tx >= SCRW - 34 && tx <= SCRW - 2 && ty >= 128 && ty <= 154) {
          cornerStretchPct = min(15.0f, cornerStretchPct + 0.5f);
          hasUnsavedChanges = true;
          refreshStretchUI();
          beep(8);
          lastBtnTap = millis();
        } else if (tx >= 4 && tx <= 4 + btnHalfW && ty >= SCRH - 42 && ty <= SCRH - 4) {
          beep(20);
          waitRelease();
          runStage1Calibration(true);
          touch.setFilters(0, 0);
          done = true;
          runStage2FilterSliders();
          return;
        } else if (tx >= 8 + btnHalfW && tx <= SCRW - 4 && ty >= SCRH - 42 && ty <= SCRH - 4) {
          beep(20);
          done = true;
        }
      }
    }
    delay(8);
  }

  uint16_t sxMin, sxMax, syMin, syMax;
  applyStretchedCalibrationRAM(sxMin, sxMax, syMin, syMax);
  touch.setFilters(activeMinP, activeJump);
  waitRelease();
}

static void commitSettingsToNVS() {
  setBacklight(255);
  uint16_t sxMin, sxMax, syMin, syMax;
  applyStretchedCalibrationRAM(sxMin, sxMax, syMin, syMax);
  touch.saveCalibration();

  touch.setFilters(activeMinP, activeJump);
  touch.saveFilters();
  hasUnsavedChanges = false;

  beep(35);
  delay(40);
  beep(60);
  tft.fillRoundRect(10, SCRH / 2 - 25, SCRW - 20, 50, 6, C_BTN_START);
  tft.drawRoundRect(10, SCRH / 2 - 25, SCRW - 20, 50, 6, C_ACCENT);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_TEXT, C_BTN_START);
  tft.drawString("SAVED TO NVS FLASH!", SCRW / 2, SCRH / 2 - 6, 1);
  tft.drawString("Settings are permanent.", SCRW / 2, SCRH / 2 + 10, 1);
  delay(1200);
}

static void drawStage3Canvas() {
  setBacklight(255);
  tft.fillScreen(C_BG);

  tft.fillRect(0, 0, SCRW, 24, C_PANEL);
  
  int b1 = SCRW / 4;
  drawButton(1,         2, b1 - 2, 20, "CLEAR",  C_BTN_RED, C_TEXT);
  drawButton(b1 + 1,    2, b1 - 2, 20, "< TUNE", C_PANEL, C_TEXT);
  drawButton(b1*2 + 1,  2, b1 - 2, 20, "RE-CAL", C_BTN_SEL, C_TEXT);
  drawButton(b1*3 + 1,  2, b1 - 2, 20, hasUnsavedChanges ? "SAVE*" : "SAVED", hasUnsavedChanges ? C_BTN_WARN : C_BTN_START, C_TEXT);

  const int boxSz = 20;
  tft.drawRect(0, 25, boxSz, boxSz, TFT_RED);
  tft.drawPixel(0, 25, TFT_YELLOW);

  tft.drawRect(SCRW - boxSz, 25, boxSz, boxSz, TFT_RED);
  tft.drawPixel(SCRW - 1, 25, TFT_YELLOW);

  tft.drawRect(0, SCRH - boxSz, boxSz, boxSz, TFT_RED);
  tft.drawPixel(0, SCRH - 1, TFT_YELLOW);

  tft.drawRect(SCRW - boxSz, SCRH - boxSz, boxSz, boxSz, TFT_RED);
  tft.drawPixel(SCRW - 1, SCRH - 1, TFT_YELLOW);
}

// ── Stage 0: Hardware Inspector & Orientation Selector ──────────────────────
static int runOrientationSelector() {
  setBacklight(255);
  tft.fillScreen(C_BG);
  
  const char* panelName = display.panelName();
  int detectedW = tft.width();
  int detectedH = tft.height();
  bool hasSavedCal = touch.loadCalibration();

  int selectedRot = 3;

  auto drawMenu = [&](int sel) {
    setBacklight(255);
    tft.fillScreen(C_BG);

    tft.fillRect(0, 0, SCRW, 26, C_PANEL);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(C_ACCENT, C_PANEL);
    tft.drawString("CYD HARDWARE INSPECTOR", SCRW / 2, 13, 1);

    tft.fillRoundRect(6, 30, SCRW - 12, 58, 4, C_CARD);
    tft.drawRoundRect(6, 30, SCRW - 12, 58, 4, C_BORDER);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(C_TEXT, C_CARD);
    
    char line1[60], line2[60], line3[60];
    snprintf(line1, sizeof(line1), " Board: %s", getDetectedBoardName());
    snprintf(line2, sizeof(line2), " Panel: %s | Res: %dx%d", panelName ? panelName : "Generic", SCRW, SCRH);
    snprintf(line3, sizeof(line3), " Touch: XPT2046 | NVS: %s", hasSavedCal ? "Saved" : "New");

    tft.drawString(line1, 10, 36, 1);
    tft.drawString(line2, 10, 51, 1);
    tft.drawString(line3, 10, 66, 1);

    int btnW = SCRW - 40;
    int btnH = 32;
    int btnX = 20;

    uint16_t bgL = (sel == 3) ? C_BTN_SEL : C_BTN_UNSEL;
    uint16_t bordL = (sel == 3) ? C_ACCENT : C_BORDER;
    tft.fillRoundRect(btnX, 94, btnW, btnH, 6, bgL);
    tft.drawRoundRect(btnX, 94, btnW, btnH, 6, bordL);
    tft.setTextColor(sel == 3 ? C_TEXT : C_MUTED, bgL);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Landscape (Rotation 3)", SCRW / 2, 94 + btnH / 2, 1);

    uint16_t bgP = (sel == 0) ? C_BTN_SEL : C_BTN_UNSEL;
    uint16_t bordP = (sel == 0) ? C_ACCENT : C_BORDER;
    tft.fillRoundRect(btnX, 132, btnW, btnH, 6, bgP);
    tft.drawRoundRect(btnX, 132, btnW, btnH, 6, bordP);
    tft.setTextColor(sel == 0 ? C_TEXT : C_MUTED, bgP);
    tft.drawString("Portrait (Rotation 0)", SCRW / 2, 132 + btnH / 2, 1);

    tft.fillRoundRect(btnX, 174, btnW, 36, 6, C_BTN_START);
    tft.drawRoundRect(btnX, 174, btnW, 36, 6, C_ACCENT);
    tft.setTextColor(C_TEXT, C_BTN_START);
    tft.drawString("CONFIRM & START (Hold BOOT)", SCRW / 2, 192, 1);
  };

  drawMenu(selectedRot);
  waitRelease();

  while (true) {
    uint16_t x = 0, y = 0;
    if (safeReadTouch(x, y)) {
      int btnW = SCRW - 40;
      int btnX = 20;

      if (x >= btnX && x <= btnX + btnW && y >= 94 && y <= 126) {
        selectedRot = 3;
        tft.setRotation(3);
        touch.setRotation(3);
        SCRW = tft.width(); SCRH = tft.height();
        drawMenu(selectedRot);
        beep(10);
        waitRelease();
      } else if (x >= btnX && x <= btnX + btnW && y >= 132 && y <= 164) {
        selectedRot = 0;
        tft.setRotation(0);
        touch.setRotation(0);
        SCRW = tft.width(); SCRH = tft.height();
        drawMenu(selectedRot);
        beep(10);
        waitRelease();
      } else if (x >= btnX && x <= btnX + btnW && y >= 174 && y <= 210) {
        beep(25);
        waitRelease();
        return selectedRot;
      }
    }

    if (digitalRead(CAL_BOOT_PIN) == LOW) {
      uint32_t pressStart = millis();
      while (digitalRead(CAL_BOOT_PIN) == LOW && (millis() - pressStart < 800)) {
        delay(10);
      }
      if (millis() - pressStart >= 800) {
        beep(25);
        while (digitalRead(CAL_BOOT_PIN) == LOW) delay(10);
        waitRelease();
        return selectedRot;
      } else {
        selectedRot = (selectedRot == 3) ? 0 : 3;
        tft.setRotation(selectedRot);
        touch.setRotation(selectedRot);
        SCRW = tft.width(); SCRH = tft.height();
        drawMenu(selectedRot);
        beep(15);
        while (digitalRead(CAL_BOOT_PIN) == LOW) delay(10);
        waitRelease();
      }
    }
    delay(10);
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(CAL_BOOT_PIN, INPUT_PULLUP);
  pinMode(TOUCH_IRQ_PIN, INPUT);

  // Initialize display and touch with AMK_TFT auto-detection
  tft.init();
  tft.setBGR(false); // True RGB subpixel mode (Red=Red, Blue=Blue, Cyan=Cyan)
  tft.setRotation(3);
  setBacklight(255);
  SCRW = tft.width();
  SCRH = tft.height();

  touch.begin(display);
  touch.setRotation(3);
  NATW = touch.nativeWidth();
  NATH = touch.nativeHeight();

  TARGET_ROTATION = runOrientationSelector();

  tft.setRotation(TARGET_ROTATION);
  touch.setRotation(TARGET_ROTATION);
  SCRW = tft.width();
  SCRH = tft.height();
  NATW = touch.nativeWidth();
  NATH = touch.nativeHeight();

  Serial.printf("\n=== Universal TouchCalibrate for %s (Rotation %d: %dx%d) ===\n",
                getDetectedBoardName(), TARGET_ROTATION, SCRW, SCRH);

  bool hasSavedCal = touch.loadCalibration();
  if (hasSavedCal) {
    touch.getCalibration(base_xMin, base_xMax, base_yMin, base_yMax);
  }

  if (touch.loadFilters()) {
    touch.getFilters(activeMinP, activeJump);
    if (activeMinP == 0) activeMinP = DEF_MINP;
  } else {
    activeMinP = DEF_MINP;
    activeJump = DEF_JUMP;
  }

  runStage1Calibration(hasSavedCal);
  runStage2FilterSliders();
  drawStage3Canvas();
}

void loop() {
  uint16_t x = 0, y = 0;
  if (safeReadTouch(x, y)) {
    int b1 = SCRW / 4;
    if (y <= 24) {
      if (x >= 1 && x <= b1) {
        beep(10);
        drawStage3Canvas();
        waitRelease();
      } else if (x >= b1 + 1 && x <= b1 * 2) {
        beep(15);
        waitRelease();
        runStage2FilterSliders();
        drawStage3Canvas();
      } else if (x >= b1 * 2 + 1 && x <= b1 * 3) {
        beep(20);
        waitRelease();
        runStage1Calibration(true);
        runStage2FilterSliders();
        drawStage3Canvas();
      } else if (x >= b1 * 3 + 1 && x <= SCRW) {
        waitRelease();
        commitSettingsToNVS();
        drawStage3Canvas();
      }
    } else {
      // Draw the cyan tracking dot cleanly with zero text or background boxes blocking your view
      tft.fillCircle(x, y, 2, C_ACCENT);
    }
  }

  if (digitalRead(CAL_BOOT_PIN) == LOW) {
    uint32_t pressStart = millis();
    while (digitalRead(CAL_BOOT_PIN) == LOW) {
      if (millis() - pressStart > 1200) {
        beep(30);
        while (digitalRead(CAL_BOOT_PIN) == LOW) delay(10);
        runStage2FilterSliders();
        drawStage3Canvas();
        return;
      }
      delay(10);
    }
    drawStage3Canvas();
  }
}