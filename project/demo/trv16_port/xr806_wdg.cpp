#include <stdint.h>
#include <stdio.h>

#include "driver/chip/hal_wdg.h"
#include "kernel/os/os_thread.h"
#include "kernel/os/os_timer.h"

// Watchdog über den originalen XR806-SDK-Treiber füttern.
// Initialisiert oder aktiviert den Watchdog nicht.
static volatile uint32_t s_feed_count = 0;

extern "C" void xr806_wdt_feed(void) {
    HAL_WDG_Feed();
    s_feed_count = s_feed_count + 1;
}

// Once the application feeds the watchdog from its main loop, the SDK timer in
// platform_init.c stops feeding it, so a hung main loop resets the chip.
static volatile bool s_app_feeds_wdt = false;

// Hang monitor: an OS timer (the timer task kept running in every hang seen so far)
// checks that the main loop still feeds the watchdog. After HANG_DUMP_SECONDS
// without a feed it prints the saved stack of the main loop task, so the call it
// waits in can be found with addr2line, and restarts the chip.
static constexpr uint32_t HANG_DUMP_SECONDS = 8;
static constexpr uint32_t HANG_DUMP_WORDS = 96;
static OS_Timer_t s_hang_timer;
static OS_ThreadHandle_t s_main_task = nullptr;
static uint32_t s_last_feed_count = 0;
static uint32_t s_stalled_seconds = 0;

static void hang_monitor(void * /*arg*/) {
    const uint32_t count = s_feed_count;
    if (count != s_last_feed_count) {
        s_last_feed_count = count;
        s_stalled_seconds = 0;
        return;
    }
    if (++s_stalled_seconds < HANG_DUMP_SECONDS)
        return;

    // FreeRTOS keeps the saved stack pointer as the first member of the task control block.
    const uint32_t *sp = *reinterpret_cast<uint32_t *const *>(s_main_task);
    printf("\n[HANG] main loop has not fed the watchdog for %u s; saved stack of task %p at %p:\n",
           (unsigned) s_stalled_seconds, s_main_task, sp);
    for (uint32_t i = 0; i < HANG_DUMP_WORDS; i += 4) {
        printf("[HANG] %p: %08lx %08lx %08lx %08lx\n", sp + i, (unsigned long) sp[i], (unsigned long) sp[i + 1],
               (unsigned long) sp[i + 2], (unsigned long) sp[i + 3]);
    }
    printf("[HANG] restarting\n");
    HAL_WDG_Reboot();
}

extern "C" void xr806_wdt_take_over(void) {
    HAL_WDG_Feed();
    s_main_task = OS_ThreadGetCurrentHandle();
    s_app_feeds_wdt = true;
    OS_TimerSetInvalid(&s_hang_timer);
    if (OS_TimerCreate(&s_hang_timer, OS_TIMER_PERIODIC, hang_monitor, nullptr, 1000) == OS_OK)
        OS_TimerStart(&s_hang_timer);
}

extern "C" int platform_wdg_timer_feeds(void) {
    return s_app_feeds_wdt ? 0 : 1;
}

// Vollständiger XR806-Systemneustart über den SDK-Watchdog
extern "C" void xr806_reboot(void) {
    HAL_WDG_Reboot();
}
