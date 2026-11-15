/*******************************************************************************
 * Terminal_Console_Demo.ino - Responsive Hardware-Scrolling Console Showcase
 *
 * Demonstrates the ultra-fast AMK_Terminal companion in AMK_CYD_Universal_TFT:
 *   - Zero-overhead 5 KB static RAM character grid (no heap allocation)
 *   - Native ILI9341 / ST7789 / ST7796 display hardware vertical scrolling
 *   - Fully responsive layout: dynamically adapts text, headers, dividers,
 *     and telemetry formatting to the exact screen columns & rows (15 cols on
 *     240x320 vs 20 cols on 320x480) with zero text wrapping or overflow!
 *   - Hardware diagnostics & Battery ADC telemetry (GPIO 34)
 *   - Smooth ambient light LDR auto-dimming (tft.adjustBrightness())
 *   - Tactile audio feedback via playBeep()
 *   - Interactive character-cell touch grid buttons
 *
 * Works across all ESP32 CYD display variants (2.4", 2.8", 3.2", 3.5", 4.0").
 * Note: Hardware scrolling operates along the panel's native long axis, so
 *       terminals run in portrait orientation (rotation 0 or 2).
 ******************************************************************************/

#include <AMK_CYD_Universal_TFT.h>
#include <AMK_Terminal.h>

AMK_display  display;
AMK_touch    touch;
AMK_Terminal term;
TFT_eSPI     tft;

// Timing and loop state
unsigned long lastLogTime = 0;
unsigned long logCount = 0;

// Dynamic responsive divider line matching exact screen width/columns
void printDivider(char c, uint16_t fg, uint16_t bg = CYDR_BLACK) {
    term.setTextColor(fg, bg);
    int cols = term.columns();
    for (int i = 0; i < cols; i++) {
        term.print(c);
    }
    term.println();
}

// Dynamic centered text matching exact screen width/columns
void printCentered(const char* text, uint16_t fg, uint16_t bg = CYDR_BLACK) {
    int cols = term.columns();
    int len = (int)strlen(text);
    term.setTextColor(fg, bg);
    if (len >= cols) {
        for (int i = 0; i < cols; i++) term.print(text[i]);
    } else {
        int pad = (cols - len) / 2;
        for (int i = 0; i < pad; i++) term.print(' ');
        term.print(text);
    }
    term.println();
}

void setup() {
    Serial.begin(115200);
    delay(200);

    // 1. Initialize Display Subsystem & Hardware Terminal
    // Rotation 0 = Portrait (Hardware scrolling operates on panel long axis)
    term.begin(display, 0);

    // 2. Attach Touch Screen Controller for Character-Cell Touch Detection
    touch.begin(display);
    touch.loadCalibration();
    touch.loadFilters();
    term.attachTouch(touch);

    // 3. Play Welcome Beep Tone on Speaker
    tft.playBeep(2400, 80);

    // 4. Responsive Header Banner (adapts to columns: 15 on 240x320, 20 on 320x480)
    int cols = term.columns();
    printDivider('=', CYDR_CYAN);
    if (cols >= 20) {
        printCentered("AMK CYD TERMINAL", CYDR_YELLOW);
    } else {
        printCentered("AMK TERMINAL", CYDR_YELLOW);
    }
    printDivider('=', CYDR_CYAN);

    // 5. Hardware Diagnostics & Screen Geometry Telemetry
    term.setTextColor(CYDR_WHITE, CYDR_BLACK);
    term.printf("Board: %s\n", display.modelName());
    term.printf("Panel: %s\n", display.panelName());
    term.printf("Grid:  %d x %d chars\n", cols, term.rows());

    // 6. Battery / Power Telemetry (calibrated ADC on GPIO 34)
    if (tft.isBatteryPowered()) {
        term.setTextColor(CYDR_GREEN, CYDR_BLACK);
        if (cols >= 20) {
            term.printf("Bat:   %.2fV (%u%%)\n", tft.getBatteryVoltage(), tft.getBatteryPercentage());
        } else {
            term.printf("Bat:   %.1fV (%u%%)\n", tft.getBatteryVoltage(), tft.getBatteryPercentage());
        }
    } else {
        term.setTextColor(CYDR_MAGENTA, CYDR_BLACK);
        term.println("Power: USB (5V)");
    }

    // 7. Interactive touch instructions
    term.setTextColor(CYDR_GREEN, CYDR_BLACK);
    if (cols >= 20) {
        term.println("Touch cell for click!");
    } else {
        term.println("Tap: Touch cells");
    }
    printDivider('-', CYDR_WHITE);
}

void loop() {
    // Smooth LDR ambient light auto-dimming
    tft.adjustBrightness();

    int cols = term.columns();

    // 1. Check for Interactive Character-Cell Touch
    int16_t touchCol = -1;
    int16_t touchRow = -1;
    if (term.pollTouch(touchCol, touchRow)) {
        tft.playBeep(2400, 30); // Fast tactile touch click
        term.setTextColor(CYDR_YELLOW, CYDR_BLUE);
        if (cols >= 20) {
            term.printf(" TOUCH [C:%2d, R:%2d] \n", touchCol, touchRow);
        } else {
            term.printf(" TOUCH C:%d R:%d \n", touchCol, touchRow);
        }
        term.setTextColor(CYDR_WHITE, CYDR_BLACK);
        delay(120); // Debounce
    }

    // 2. Stream Hardware-Scrolled Telemetry Logs
    if (millis() - lastLogTime >= 800) {
        lastLogTime = millis();
        logCount++;

        uint32_t freeHeapK = ESP.getFreeHeap() / 1024;
        term.setTextColor(CYDR_GREEN, CYDR_BLACK);

        if (cols >= 20) {
            // 20+ columns mode (320x480 panels, etc.): Fits in exactly 19-20 chars on 1 line
            term.printf("[%04lu] %3luK ", logCount, (unsigned long)freeHeapK);
            if (tft.isBatteryPowered()) {
                term.setTextColor(CYDR_CYAN, CYDR_BLACK);
                term.printf("B:%.2fV\n", tft.getBatteryVoltage());
            } else {
                term.setTextColor(CYDR_WHITE, CYDR_BLACK);
                term.printf("Up:%lus\n", millis() / 1000);
            }
        } else {
            // 15 columns mode (240x320 panels, etc.): Fits in exactly 13-14 chars on 1 line
            term.printf("#%lu %luK ", logCount, (unsigned long)freeHeapK);
            if (tft.isBatteryPowered()) {
                term.setTextColor(CYDR_CYAN, CYDR_BLACK);
                term.printf("B:%.1fV\n", tft.getBatteryVoltage());
            } else {
                term.setTextColor(CYDR_WHITE, CYDR_BLACK);
                term.printf("U:%lus\n", millis() / 1000);
            }
        }
    }
}
