#include <stdint.h>

/*
 * Replacement for PHY_RF_DPD_RecipFunc() from libxrwireless_phy.a (phy_rf.o).
 *
 * The function computes a normalised reciprocal of x for the RF calibration code
 * (RX IQ calibration and DPD LUT generation): x is shifted left until it is above
 * 2^29, *exp counts the shifts (starting at 3) and *out gets about 2^60 / x from two
 * Newton steps. For x == 0 the original reports "[FW-DBG] DbgId = 21 / 0x141" and
 * then shifts 0 forever. The calibration runs in a priority 5 task, so the endless
 * loop starves the main loop and the network stack until the watchdog resets the
 * chip. This happens when the RX IQ calibration measures no signal power.
 *
 * Here x == 0 gives *out = 0, which makes the callers compute a zero correction,
 * and every other input gives the same result as the original code (checked by
 * running both on the same inputs in an emulator).
 *
 * The library copy linked into the image has this symbol made weak (see gcc/Makefile).
 */

extern void APP_SendDebugIndication(int id, int code);

void PHY_RF_DPD_RecipFunc(uint32_t x, uint32_t *out, uint8_t *exp)
{
    uint8_t e = 3;

    if (x == 0) {
        APP_SendDebugIndication(21, 0x141);
        *exp = e;
        *out = 0;
        return;
    }
    while (x <= 0x20000000u) {
        x <<= 1;
        e++;
    }
    *exp = e;

    const uint32_t y0 = 0x57851eb8u + (x >> 3) - x;
    const uint64_t err0 = (1ull << 30) - (((uint64_t)x * y0) >> 30);
    const uint32_t y1 = (uint32_t)((err0 * y0) >> 30);
    const uint64_t err1 = (1ull << 30) - (((uint64_t)x * y1) >> 29);
    *out = (uint32_t)((err1 * y1) >> 30);
}
