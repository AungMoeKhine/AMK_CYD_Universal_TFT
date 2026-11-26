#ifndef AMK_RESPONSIVE_UI_H
#define AMK_RESPONSIVE_UI_H

#include "Free_Fonts.h"

// Hardware UI Tiers according to AMK_CYD_Responsive_Design_Standard
enum AMK_UiTier : uint8_t {
    UI_TIER_COMPACT = 0, // 2.4", 2.8", 3.2" (320x240 or 240x320)
    UI_TIER_MID     = 1, // 3.5", 4.0"       (480x320 or 320x480)
    UI_TIER_LARGE   = 2  // 4.3", 5.0", 7.0" (800x480 or 480x800)
};

// Unified Responsive Font & UI Profile
struct AMK_FontProfile {
    AMK_UiTier     tier;
    int16_t        screenWidth;
    int16_t        screenHeight;
    bool           isCompact;
    bool           isMid;
    bool           isLarge;
    
    // Proportional Typography Pointers
    const GFXfont* titleFont;       // Prominent section titles & header
    const GFXfont* titleFontBold;   // Bold titles
    const GFXfont* bodyFont;        // Regular body & descriptions
    const GFXfont* bodyFontBold;    // Bold body & indicators
    const GFXfont* valueFont;       // Telemetry data & numbers
    const GFXfont* valueFontBold;   // Bold telemetry data
    const GFXfont* largeFont;       // Large subtitles or prominent numbers
    const GFXfont* heroFont;        // Giant digits (clock time, main temp)
    const GFXfont* tinyFont;        // Small subtitles / badges
    const GFXfont* tinyFontBold;    // Small bold tags

    // Built-in GLCD Font 1 Sizing
    uint8_t        labelFontSize;   // Recommended GLCD Font 1 text size for labels (1)
    uint8_t        headerFontSize;  // GLCD Font 1 text size for header (1 or 2)
    uint8_t        bodyFontSize;    // GLCD Font 1 text size for regular text (1 or 2)
    
    // Layout and Spacing Standard
    int16_t        marginOuter;     // Screen outer margin (4px for compact, 8px for mid, 12px for large)
    int16_t        cardGap;         // Gap between cards (4px for compact, 6px for mid, 8px for large)
    int16_t        cardRadius;      // Corner radius of cards (4px for compact, 8px for mid, 10px for large)
    int16_t        headerHeight;    // Top status/header bar height (28px for compact, 34-44px for mid, 48px for large)
};

// Helper function to resolve Font & UI profile dynamically at runtime
inline AMK_FontProfile AMK_getFontProfile(int16_t width, int16_t height) {
    AMK_FontProfile p = {};
    p.screenWidth = width;
    p.screenHeight = height;

    int16_t maxDim = (width > height) ? width : height;

    if (maxDim >= 700) {
        // Tier L: 4.3", 5.0", 7.0" (800x480)
        p.tier           = UI_TIER_LARGE;
        p.isCompact      = false;
        p.isMid          = false;
        p.isLarge        = true;

        p.titleFont      = FSS18;
        p.titleFontBold  = FSSB18;
        p.bodyFont       = FSS12;
        p.bodyFontBold   = FSSB12;
        p.valueFont      = FSS18;
        p.valueFontBold  = FSSB18;
        p.largeFont      = FSSB24;
        p.heroFont       = FSSB24;
        p.tinyFont       = FSS9;
        p.tinyFontBold   = FSSB9;

        p.labelFontSize  = 1;
        p.headerFontSize = 2;
        p.bodyFontSize   = 2;

        p.marginOuter    = 12;
        p.cardGap        = 8;
        p.cardRadius     = 10;
        p.headerHeight   = 48;
    } else if (maxDim >= 400) {
        // Tier M: 3.5", 4.0" (480x320) - Current verified baseline!
        p.tier           = UI_TIER_MID;
        p.isCompact      = false;
        p.isMid          = true;
        p.isLarge        = false;

        p.titleFont      = FSS12;
        p.titleFontBold  = FSSB12;
        p.bodyFont       = FSS12;
        p.bodyFontBold   = FSSB12;
        p.valueFont      = FSS12;
        p.valueFontBold  = FSSB12;
        p.largeFont      = FSSB18;
        p.heroFont       = FSSB24;
        p.tinyFont       = FSS9;
        p.tinyFontBold   = FSSB9;

        p.labelFontSize  = 1;
        p.headerFontSize = 1;
        p.bodyFontSize   = 1;

        p.marginOuter    = 8;
        p.cardGap        = 6;
        p.cardRadius     = 8;
        p.headerHeight   = 34;
    } else {
        // Tier S: 2.4", 2.8", 3.2" (320x240) - Compact QVGA
        p.tier           = UI_TIER_COMPACT;
        p.isCompact      = true;
        p.isMid          = false;
        p.isLarge        = false;

        p.titleFont      = FSS9;
        p.titleFontBold  = FSSB9;
        p.bodyFont       = FSS9;
        p.bodyFontBold   = FSSB9;
        p.valueFont      = FSS9;
        p.valueFontBold  = FSSB9;
        p.largeFont      = FSSB12;
        p.heroFont       = FSSB18;
        p.tinyFont       = FSS9;
        p.tinyFontBold   = FSSB9;

        p.labelFontSize  = 1;
        p.headerFontSize = 1;
        p.bodyFontSize   = 1;

        p.marginOuter    = 4;
        p.cardGap        = 4;
        p.cardRadius     = 6;
        p.headerHeight   = 28;
    }

    return p;
}

#endif // AMK_RESPONSIVE_UI_H
