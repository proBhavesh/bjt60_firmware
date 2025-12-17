/*
 * BJT60 Presence Detection Firmware - SIMPLE DEBUG
 */

#include "clock.h"
#include "gpio.h"
#include "spi.h"
#include "avian_radar.h"
#include "presence_detection.h"

static presence_ctx_t presence_ctx;

/*
 * Disable watchdog
 */
static void disable_watchdog(void)
{
    volatile uint32_t *WDT_MR = (volatile uint32_t *)0x400E1854;
    *WDT_MR = (1 << 15);
}

/*
 * Simple blink using direct register access
 */
static void blink(int count)
{
    for (int i = 0; i < count; i++) {
        *((volatile uint32_t *)(0x400E1434)) = (1 << 5);  /* ON */
        for (volatile int j = 0; j < 800000; j++);
        *((volatile uint32_t *)(0x400E1430)) = (1 << 5);  /* OFF */
        for (volatile int j = 0; j < 800000; j++);
    }
    /* Pause after pattern */
    for (volatile int j = 0; j < 2000000; j++);
}

/*
 * Main
 */
int main(void)
{
    disable_watchdog();

    /* Enable LED */
    *((volatile uint32_t *)(0x400E0610)) = (1 << 16);
    *((volatile uint32_t *)(0x400E1400)) = (1 << 5);
    *((volatile uint32_t *)(0x400E1410)) = (1 << 5);

    /* CHECKPOINT 1 */
    blink(1);

    clock_init();

    /* CHECKPOINT 2 */
    blink(2);

    gpio_init();

    /* CHECKPOINT 3 */
    blink(3);

    spi_init();

    /* CHECKPOINT 4 */
    blink(4);

    radar_init();

    /* CHECKPOINT 5 */
    blink(5);

    presence_init(&presence_ctx);
    radar_start();

    /* SUCCESS - continuous slow blink */
    while (1) {
        *((volatile uint32_t *)(0x400E1434)) = (1 << 5);
        for (volatile int j = 0; j < 3000000; j++);
        *((volatile uint32_t *)(0x400E1430)) = (1 << 5);
        for (volatile int j = 0; j < 3000000; j++);
    }

    return 0;
}
