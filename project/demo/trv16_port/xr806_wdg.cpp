#include "driver/chip/hal_wdg.h"

// Watchdog über den originalen XR806-SDK-Treiber füttern.
// Initialisiert oder aktiviert den Watchdog nicht.
extern "C" void xr806_wdt_feed(void) {
    HAL_WDG_Feed();
}

// Once the application feeds the watchdog from its main loop, the SDK timer in
// platform_init.c stops feeding it, so a hung main loop resets the chip.
static volatile bool s_app_feeds_wdt = false;

extern "C" void xr806_wdt_take_over(void) {
    HAL_WDG_Feed();
    s_app_feeds_wdt = true;
}

extern "C" int platform_wdg_timer_feeds(void) {
    return s_app_feeds_wdt ? 0 : 1;
}

// Vollständiger XR806-Systemneustart über den SDK-Watchdog
extern "C" void xr806_reboot(void) {
    HAL_WDG_Reboot();
}
