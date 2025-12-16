#pragma once

#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"

namespace ultrasonic01 {

struct Pins {
    gpio_num_t trig;
    gpio_num_t echo;
};

struct Timings {
    int trigger_high_us = 10;
    int settle_low_us = 2;
    int timeout_us = 300000; // 300 ms
};

inline void init_pins(const Pins &pins) {
    gpio_reset_pin(pins.trig);
    gpio_set_direction(pins.trig, GPIO_MODE_OUTPUT);
    gpio_set_level(pins.trig, 0);

    gpio_reset_pin(pins.echo);
    gpio_set_direction(pins.echo, GPIO_MODE_INPUT);
}

// Returns distance in cm, or -1 on timeout/error
inline int measure_cm(const Pins &pins, const Timings &t = Timings()) {
    gpio_set_level(pins.trig, 0);
    esp_rom_delay_us(t.settle_low_us);
    gpio_set_level(pins.trig, 1);
    esp_rom_delay_us(t.trigger_high_us);
    gpio_set_level(pins.trig, 0);

    int64_t start_us = esp_timer_get_time();
    // wait echo HIGH
    while (gpio_get_level(pins.echo) == 0) {
        if ((esp_timer_get_time() - start_us) > t.timeout_us) return -1;
    }
    int64_t echo_start = esp_timer_get_time();
    while (gpio_get_level(pins.echo) == 1) {
        if ((esp_timer_get_time() - echo_start) > t.timeout_us) return -1;
    }
    int64_t echo_end = esp_timer_get_time();
    int64_t pulse_us = echo_end - echo_start;
    // HC-SR04: cm ≈ us/58, rounded
    return (int)((pulse_us + 29) / 58);
}

// Utility: median of 3 ints (for simple noise rejection)
inline int median3(int a, int b, int c) {
    if (a > b) { int t=a;a=b;b=t; }
    if (b > c) { int t=b;b=c;c=t; }
    if (a > b) { int t=a;a=b;b=t; }
    return b;
}

} // namespace ultrasonic01
