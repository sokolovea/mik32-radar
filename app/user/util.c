#include "mik32_hal_usart.h"
#include "mik32_hal_i2c.h"
#include "lcd_driver.h"
#include "mik32_hal_gpio.h"
#include "delay.h"
#include "util.h"

#include "epic.h"
#include "gpio.h"
#include "gpio_irq.h"
#include "mik32_hal_irq.h"
#include "mik32_memory_map.h"
#include "pad_config.h"
#include "power_manager.h"
#include "scr1_timer.h"
#include "timer32.h"
#include "wakeup.h"
#include "xprintf.h"

#define SYSTEM_FREQ_HZ (32000000UL)
#define TICKS_PER_MS (SYSTEM_FREQ_HZ / 1000U)

#define MIN_ANGLE_PER_SEC (1)
#define MAX_ANGLE_PER_SEC (90)
#define ONE_ANGLE_ROTATE_MS (5)

void update_servo_angle(int angle)
{
    int ticks_angle = TICKS_PER_MS / 2 + angle * 2 * TICKS_PER_MS / 180;
    if (ticks_angle < TICKS_PER_MS / 2) {
        ticks_angle = TICKS_PER_MS / 2;
    }
    else if (ticks_angle > 2 * TICKS_PER_MS + TICKS_PER_MS / 2) {
        ticks_angle = 2 * TICKS_PER_MS;
    }
    #ifndef TEST
    TIMER32_1->CHANNELS[1].OCR = ticks_angle;
    #endif
}

int get_delay_ms_by_angle_per_second(int angle_per_second) {
    if (angle_per_second <= 0) {
        angle_per_second = MIN_ANGLE_PER_SEC;
    } else if (angle_per_second > MAX_ANGLE_PER_SEC) {
        angle_per_second = MAX_ANGLE_PER_SEC;
    }
    return MAX_ANGLE_PER_SEC * ONE_ANGLE_ROTATE_MS / angle_per_second;
}