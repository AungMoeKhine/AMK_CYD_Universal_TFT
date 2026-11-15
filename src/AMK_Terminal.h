#pragma once
#ifndef AMK_TERMINAL_H
#define AMK_TERMINAL_H
// =============================================================================
// AMK_Terminal.h - Ultra-fast Hardware-Scrolling Text Terminal for AMK_CYD_Universal_TFT
//                  Consumes only 5 KB RAM, uses ILI9341/ST7789/ST7796 hardware
//                  vertical scrolling (VSCRDEF/VSCRSADD), UTF-8, ANSI color
//                  support, and character-cell touch detection.
//
// Usage in sketch:
//     #include <AMK_CYD_Universal_TFT.h>
//     #include <AMK_Terminal.h>
//
//     AMK_display  display;
//     AMK_Terminal term;
//
//     void setup() {
//         term.begin(display);
//         term.println("Hello, CYD Terminal!");
//     }
// =============================================================================

#include "chipguy_CYD_resistive_terminal.h"

// Universal Type Aliases
using AMK_Terminal      = chipguy_CYD_resistive_terminal;
using AMK_CYD_Terminal  = chipguy_CYD_resistive_terminal;
using CYD_Terminal      = chipguy_CYD_resistive_terminal;

#endif // AMK_TERMINAL_H
