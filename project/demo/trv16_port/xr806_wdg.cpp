#include <stdint.h>
#include <stdio.h>

#include "driver/chip/hal_uart.h"
#include "driver/chip/hal_wdg.h"
#include "kernel/os/os_thread.h"
#include "kernel/os/os_timer.h"

extern "C" {
#include "FreeRTOS.h"
#include "task.h"
}

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

// Writes straight to the UART2 console. printf() takes the stdio lock, which the task that
// printed the WLAN firmware fault may still hold, and then the dump never appears.
static void hang_write(const char *text) {
    UART_T *uart = HAL_UART_GetInstance(UART2_ID);
    for (; *text != '\0'; ++text) {
        while (!HAL_UART_IsTxReady(uart)) {
        }
        HAL_UART_PutTxData(uart, static_cast<uint8_t>(*text));
    }
}

static void hang_dump_stack(const uint32_t *sp, uint32_t words) {
    char line[96];
    for (uint32_t i = 0; i < words; i += 4) {
        snprintf(line, sizeof(line), "[HANG]   %p: %08lx %08lx %08lx %08lx\r\n", sp + i, (unsigned long) sp[i],
                 (unsigned long) sp[i + 1], (unsigned long) sp[i + 2], (unsigned long) sp[i + 3]);
        hang_write(line);
    }
}

static void hang_monitor(void * /*arg*/) {
    const uint32_t count = s_feed_count;
    if (count != s_last_feed_count) {
        s_last_feed_count = count;
        s_stalled_seconds = 0;
        return;
    }
    if (++s_stalled_seconds < HANG_DUMP_SECONDS)
        return;

    char line[128];
    snprintf(line, sizeof(line), "\r\n[HANG] main loop has not fed the watchdog for %u s; main task %p\r\n",
             (unsigned) s_stalled_seconds, s_main_task);
    hang_write(line);

    // All tasks with their state and saved stack. FreeRTOS keeps the saved stack pointer
    // as the first member of the task control block.
    static TaskStatus_t tasks[24];
    const UBaseType_t count_tasks = uxTaskGetSystemState(tasks, 24, nullptr);
    for (UBaseType_t t = 0; t < count_tasks; t++) {
        const uint32_t *sp = *reinterpret_cast<uint32_t *const *>(tasks[t].xHandle);
        snprintf(line, sizeof(line), "[HANG] task %-12s state %d prio %lu handle %p sp %p free %u\r\n",
                 tasks[t].pcTaskName, (int) tasks[t].eCurrentState, (unsigned long) tasks[t].uxCurrentPriority,
                 tasks[t].xHandle, sp, (unsigned) tasks[t].usStackHighWaterMark);
        hang_write(line);
        hang_dump_stack(sp, tasks[t].xHandle == s_main_task ? HANG_DUMP_WORDS : 32);
    }
    hang_write("[HANG] restarting\r\n");
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
