/*
HomeKit supported Night Light with ESP32 board.

Copyright (C) 2021  Darshan Prajapati

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "driver/rmt.h"
#include "esp_log.h"
#include "led_strip.h"

#define RMT_TX_CHANNEL RMT_CHANNEL_0

typedef struct hsp {
    uint16_t h;  // 0-360
    uint16_t s;  // 0-100
    uint16_t b;  // 0-100
} hsp_t;

static led_strip_t *strip;

static hsp_t s_hsb_val;
static uint16_t s_brightness;
static bool s_on = false;

static const char *TAG = "night_light";

/**
 * @brief Simple helper function, converting HSV color space to RGB color space
 *
 * Wiki: https://en.wikipedia.org/wiki/HSL_and_HSV
 *
 */
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

/**
 * @brief set the night_light's "HSV"
 */
static bool night_light_set_aim_hsv(uint16_t h, uint16_t s, uint16_t v)
{
	uint32_t red = 0;
	uint32_t green = 0;
	uint32_t blue = 0;

    led_strip_hsv2rgb(h, s, v, &red, &green, &blue);
    ESP_ERROR_CHECK(strip->set_pixel(strip, 0, red, green, blue));
    ESP_ERROR_CHECK(strip->refresh(strip, 100));

    return true;
}

/**
 * @brief update the night_light's state
 */
static void night_light_update()
{
    night_light_set_aim_hsv(s_hsb_val.h, s_hsb_val.s, s_hsb_val.b);
}


/**
 * @brief initialize the night_light lowlevel module
 */
void night_light_init(void)
{
	rmt_config_t config = RMT_DEFAULT_CONFIG_TX(CONFIG_EXAMPLE_RMT_TX_GPIO, RMT_TX_CHANNEL);
    // set counter clock to 40MHz
    config.clk_div = 2;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

    // install ws2812 driver
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(1, (led_strip_dev_t)config.channel);

    strip = led_strip_new_rmt_ws2812(&strip_config);

    if (!strip) {
        ESP_LOGE(TAG, "install WS2812 driver failed");
    }
    // Clear LED strip (turn off all LEDs)
    ESP_ERROR_CHECK(strip->clear(strip, 100));

    // Initial HSV values
    s_hsb_val.h = 360;
    s_hsb_val.s = 10;
    s_hsb_val.b = 50;
}

/**
 * @brief deinitialize the night_light's lowlevel module
 */
void night_light_deinit(void)
{
	// Clear LED strip (turn off all LEDs)
	ESP_ERROR_CHECK(strip->clear(strip, 100));
}

/**
 * @brief turn on/off the lowlevel night_light
 */
int night_light_set_on(bool value)
{
    ESP_LOGI(TAG, "night_light_set_on : %s", value == true ? "true" : "false");

    if (value == true) {
        s_hsb_val.b = s_brightness;
        s_on = true;
    } else {
        s_brightness = s_hsb_val.b;
        s_hsb_val.b = 0;
        s_on = false;
    }
    night_light_update();

    return 0;
}

/**
 * @brief set the saturation of the lowlevel night_light
 */
int night_light_set_saturation(float value)
{
    ESP_LOGI(TAG, "night_light_set_saturation : %f", value);

    s_hsb_val.s = value;
    if (true == s_on)
        night_light_update();

    return 0;
}

/**
 * @brief set the hue of the lowlevel night_light
 */
int night_light_set_hue(float value)
{
    ESP_LOGI(TAG, "night_light_set_hue : %f", value);

    s_hsb_val.h = value;
    if (true == s_on)
        night_light_update();

    return 0;
}

/**
 * @brief set the brightness of the lowlevel night_light
 */
int night_light_set_brightness(int value)
{
    ESP_LOGI(TAG, "night_light_set_brightness : %d", value);

    s_hsb_val.b = value;
    s_brightness = s_hsb_val.b;

    if (true == s_on)
        night_light_update();

    return 0;
}
