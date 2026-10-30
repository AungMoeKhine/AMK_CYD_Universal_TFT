#include "AMK_CYD_Universal_TFT.h"

// Default global display hardware instance with weak linkage.
// If the sketch declares its own `display`, the sketch's declaration
// takes precedence. If the sketch does not declare it, this default is used.
chipguy_CYD_resistive_display display __attribute__((weak));
