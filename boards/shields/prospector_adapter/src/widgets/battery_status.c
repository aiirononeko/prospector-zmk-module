#include "battery_status.h"

#include <zmk/display.h>
#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/split_central_status_changed.h>
#include <zmk/event_manager.h>

#include <fonts.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);
static bool initialized = false;

struct battery_update_state {
    uint8_t source;
    uint8_t level;
};

struct connection_update_state {
    uint8_t source;
    bool connected;
};

static void set_battery_value(lv_obj_t *widget, struct battery_update_state state) {
    if (!initialized) return;

    lv_obj_t *col = lv_obj_get_child(widget, state.source);
    lv_obj_t *num = lv_obj_get_child(col, 0);
    lv_obj_t *bar = lv_obj_get_child(col, 1);

    lv_bar_set_value(bar, state.level, LV_ANIM_ON);
    lv_label_set_text_fmt(num, "%d", state.level);

    if (state.level < 20) {
        lv_obj_set_style_bg_color(bar, lv_color_hex(0xD3900F), LV_PART_INDICATOR);
        lv_obj_set_style_bg_grad_color(bar, lv_color_hex(0xFFB802), LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x3d2a00), LV_PART_MAIN);
        lv_obj_set_style_text_color(num, lv_color_hex(0xFFB802), 0);
    } else {
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x606060), LV_PART_INDICATOR);
        lv_obj_set_style_bg_grad_color(bar, lv_color_hex(0xf0f0f0), LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x1a1a1a), LV_PART_MAIN);
        lv_obj_set_style_text_color(num, lv_color_hex(0xffffff), 0);
    }
}

static void set_battery_connected(lv_obj_t *widget, struct connection_update_state state) {
    if (!initialized) return;

    lv_obj_t *col = lv_obj_get_child(widget, state.source);
    lv_obj_t *num = lv_obj_get_child(col, 0);
    lv_obj_t *bar = lv_obj_get_child(col, 1);
    lv_obj_t *nc_label = lv_obj_get_child(col, 2);

    LOG_DBG("Peripheral %d %s", state.source,
            state.connected ? "connected" : "disconnected");

    if (state.connected) {
        lv_obj_fade_out(nc_label, 150, 0);
        lv_obj_fade_in(num, 150, 250);
        lv_obj_fade_in(bar, 150, 250);
    } else {
        lv_obj_fade_out(num, 150, 0);
        lv_obj_fade_out(bar, 150, 0);
        lv_obj_fade_in(nc_label, 150, 250);
    }
}

void battery_status_battery_cb(struct battery_update_state state) {
    struct zmk_widget_battery_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_battery_value(widget->obj, state);
    }
}

static struct battery_update_state battery_status_get_battery(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev =
        as_zmk_peripheral_battery_state_changed(eh);
    return (struct battery_update_state){
        .source = ev->source,
        .level = ev->state_of_charge,
    };
}

void battery_status_connection_cb(struct connection_update_state state) {
    struct zmk_widget_battery_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_battery_connected(widget->obj, state);
    }
}

static struct connection_update_state battery_status_get_connection(const zmk_event_t *eh) {
    const struct zmk_split_central_status_changed *ev =
        as_zmk_split_central_status_changed(eh);
    return (struct connection_update_state){
        .source = ev->slot,
        .connected = ev->connected,
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status_bat, struct battery_update_state,
                            battery_status_battery_cb, battery_status_get_battery);
ZMK_SUBSCRIPTION(widget_battery_status_bat, zmk_peripheral_battery_state_changed);

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status_conn, struct connection_update_state,
                            battery_status_connection_cb, battery_status_get_connection);
ZMK_SUBSCRIPTION(widget_battery_status_conn, zmk_split_central_status_changed);

int zmk_widget_battery_status_init(struct zmk_widget_battery_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);

    lv_obj_set_flex_flow(widget->obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(widget->obj,
                          LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(widget->obj, 20, LV_PART_MAIN);

    for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        lv_obj_t *col = lv_obj_create(widget->obj);
        lv_obj_set_size(col, 100, 80);
        lv_obj_set_style_bg_opa(col, 0, LV_PART_MAIN);
        lv_obj_set_style_border_width(col, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(col, 0, LV_PART_MAIN);

        /* Child 0: battery percentage */
        lv_obj_t *num = lv_label_create(col);
        lv_obj_set_style_text_font(num, &FRAC_Medium_48, 0);
        lv_obj_set_style_text_color(num, lv_color_white(), 0);
        lv_label_set_text(num, "--");
        lv_obj_align(num, LV_ALIGN_CENTER, 0, -8);
        lv_obj_set_style_opa(num, 0, 0);

        /* Child 1: thin progress bar */
        lv_obj_t *bar = lv_bar_create(col);
        lv_obj_set_size(bar, 88, 3);
        lv_obj_align(bar, LV_ALIGN_CENTER, 0, 24);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x1a1a1a), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(bar, 255, LV_PART_MAIN);
        lv_obj_set_style_radius(bar, 1, LV_PART_MAIN);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x606060), LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(bar, 255, LV_PART_INDICATOR);
        lv_obj_set_style_bg_grad_color(bar, lv_color_hex(0xf0f0f0), LV_PART_INDICATOR);
        lv_obj_set_style_bg_grad_dir(bar, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
        lv_obj_set_style_bg_dither_mode(bar, LV_DITHER_ERR_DIFF, LV_PART_INDICATOR);
        lv_obj_set_style_radius(bar, 1, LV_PART_INDICATOR);
        lv_obj_set_style_anim_time(bar, 300, 0);
        lv_bar_set_value(bar, 0, LV_ANIM_OFF);
        lv_obj_set_style_opa(bar, 0, 0);

        /* Child 2: disconnected indicator */
        lv_obj_t *nc_label = lv_label_create(col);
        lv_obj_set_style_text_font(nc_label, &FRAC_Thin_48, 0);
        lv_obj_set_style_text_color(nc_label, lv_color_hex(0x383838), 0);
        lv_label_set_text(nc_label, "--");
        lv_obj_align(nc_label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_opa(nc_label, 255, 0);
    }

    sys_slist_append(&widgets, &widget->node);

    widget_battery_status_bat_init();
    widget_battery_status_conn_init();
    initialized = true;

    return 0;
}

lv_obj_t *zmk_widget_battery_status_obj(struct zmk_widget_battery_status *widget) {
    return widget->obj;
}
