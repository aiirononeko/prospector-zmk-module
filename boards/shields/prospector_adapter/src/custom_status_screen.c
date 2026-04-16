#include <lvgl.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>

#include "widgets/battery_status.h"
#include "widgets/layer_status.h"
#include "widgets/os_mode.h"
#include "widgets/caps_word_indicator.h"

#include <fonts.h>
#include <sf_symbols.h>

#include <zmk/keymap.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static struct zmk_widget_battery_status battery_status_widget;
static struct zmk_widget_layer_status layer_status_widget;
static struct zmk_widget_os_mode os_mode_widget;
static struct zmk_widget_caps_word_indicator caps_word_indicator_widget;

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen;
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, 255, LV_PART_MAIN);

    /* Battery status (visible on default layer) */
    zmk_widget_battery_status_init(&battery_status_widget, screen);
    lv_obj_set_size(zmk_widget_battery_status_obj(&battery_status_widget), lv_pct(100), lv_pct(100));
    lv_obj_align(zmk_widget_battery_status_obj(&battery_status_widget), LV_ALIGN_CENTER, 0, 0);

    /* Layer status overlay (visible on non-default layers, covers battery) */
    zmk_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_set_size(zmk_widget_layer_status_obj(&layer_status_widget), lv_pct(100), lv_pct(100));
    lv_obj_align(zmk_widget_layer_status_obj(&layer_status_widget), LV_ALIGN_CENTER, 0, 0);

    /* OS mode indicator (bottom center, always visible alongside battery) */
    zmk_widget_os_mode_init(&os_mode_widget, screen);
    lv_obj_align(zmk_widget_os_mode_obj(&os_mode_widget), LV_ALIGN_BOTTOM_MID, 0, -24);

#ifdef CONFIG_DT_HAS_ZMK_BEHAVIOR_CAPS_WORD_ENABLED
    zmk_widget_caps_word_indicator_init(&caps_word_indicator_widget, screen);
    lv_obj_align(zmk_widget_caps_word_indicator_obj(&caps_word_indicator_widget),
                 LV_ALIGN_TOP_RIGHT, -12, 12);
#endif

    return screen;
}
