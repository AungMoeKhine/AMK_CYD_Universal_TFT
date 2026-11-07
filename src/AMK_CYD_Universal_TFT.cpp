#include "AMK_CYD_Universal_TFT.h"

#if defined(AMK_BACKEND_ESP32_S3)

static AMK_S3_Board s_amk_default_board;
AMK_S3_Board& amk_get_default_board() {
    return s_amk_default_board;
}

// Default global instances for ESP32-S3 RGB display & capacitive touch with weak linkage.
// If the sketch declares its own `display` or `touch`, the sketch declaration takes precedence.
__attribute__((weak)) AMK_S3_RGB_display display(s_amk_default_board);
#ifndef AMK_CYD_NO_GLOBAL_TOUCH
__attribute__((weak)) AMK_S3_touch       touch(s_amk_default_board);
#endif

#else

// Default global display & touch hardware instances for CYD with weak linkage.
// If the sketch declares its own `display` or `touch`, the sketch's declaration takes precedence.
chipguy_CYD_resistive_display display __attribute__((weak));
#ifndef AMK_CYD_NO_GLOBAL_TOUCH
chipguy_CYD_resistive_touch   touch   __attribute__((weak));
#endif

#endif
