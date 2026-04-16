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
    lv_obj_t *arc = lv_obj_get_child(col, 1);

    lv_arc_set_value(arc, state.level);
    lv_label_set_text_fmt(num, "%d", state.level);

    if (state.level < 20) {
        lv_obj_set_style_arc_color(arc, lv_color_hex(0xFFB802), LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(arc, lv_color_hex(0x3d2a00), LV_PART_MAIN);
        lv_obj_set_style_text_color(num, lv_color_hex(0xFFB802), 0);
    } else {
        lv_obj_set_style_arc_color(arc, lv_color_hex(0xf0f0f0), LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(arc, lv_color_hex(0x1a1a1a), LV_PART_MAIN);
        lv_obj_set_style_text_color(num, lv_color_hex(0xffffff), 0);
    }
}

static void set_battery_connected(lv_obj_t *widget, struct connection_update_state state) {
    if (!initialized) return;

    lv_obj_t *col = lv_obj_get_child(widget, state.source);
    lv_obj_t *num = lv_obj_get_child(col, 0);
    lv_obj_t *arc = lv_obj_get_child(col, 1);
    lv_obj_t *nc_label = lv_obj_get_child(col, 2);

    LOG_DBG("Peripheral %d %s", state.source,
            state.connected ? "connected" : "disconnected");

    if (state.connected) {
        lv_obj_fade_out(nc_label, 150, 0);
        lv_obj_fade_in(num, 150, 250);
        lv_obj_fade_in(arc, 150, 250);
    } else {
        lv_obj_fade_out(num, 150, 0);
        lv_obj_fade_out(arc, 150, 0);
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
        lv_obj_set_size(col, 120, 140);
        lv_obj_set_style_bg_opa(col, 0, LV_PART_MAIN);
        lv_obj_set_style_border_width(col, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(col, 0, LV_PART_MAIN);

        /* Child 0: battery percentage (centered inside the arc) */
        lv_obj_t *num = lv_label_create(col);
        lv_obj_set_style_text_font(num, &FRAC_Medium_48, 0);
        lv_obj_set_style_text_color(num, lv_color_white(), 0);
        lv_label_set_text(num, "--");
        lv_obj_align(num, LV_ALIGN_CENTER, 0, -16);
        lv_obj_set_style_opa(num, 0, 0);

        /* Child 1: circular ring gauge (Apple Watch style) */
        lv_obj_t *arc = lv_arc_create(col);
        lv_obj_set_size(arc, 108, 108);
        lv_obj_align(arc, LV_ALIGN_CENTER, 0, -16);
        lv_arc_set_range(arc, 0, 100);
        lv_arc_set_bg_angles(arc, 135, 45);
        lv_arc_set_rotation(arc, 0);
        lv_arc_set_value(arc, 0);
        lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
        lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_set_style_arc_color(arc, lv_color_hex(0x1a1a1a), LV_PART_MAIN);
        lv_obj_set_style_arc_opa(arc, 255, LV_PART_MAIN);
        lv_obj_set_style_arc_width(arc, 6, LV_PART_MAIN);
        lv_obj_set_style_arc_rounded(arc, true, LV_PART_MAIN);

        lv_obj_set_style_arc_color(arc, lv_color_hex(0xf0f0f0), LV_PART_INDICATOR);
        lv_obj_set_style_arc_opa(arc, 255, LV_PART_INDICATOR);
        lv_obj_set_style_arc_width(arc, 6, LV_PART_INDICATOR);
        lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);
        lv_obj_set_style_anim_time(arc, 400, 0);
        lv_obj_set_style_opa(arc, 0, 0);

        /* Child 2: disconnected indicator */
        lv_obj_t *nc_label = lv_label_create(col);
        lv_obj_set_style_text_font(nc_label, &FRAC_Thin_48, 0);
        lv_obj_set_style_text_color(nc_label, lv_color_hex(0x383838), 0);
        lv_label_set_text(nc_label, "--");
        lv_obj_align(nc_label, LV_ALIGN_CENTER, 0, -16);
        lv_obj_set_style_opa(nc_label, 255, 0);

        /* Child 3: side identifier (always visible, sits below the ring) */
        lv_obj_t *side = lv_label_create(col);
        lv_obj_set_style_text_font(side, &SF_Compact_Text_Medium_24, 0);
        lv_obj_set_style_text_color(side, lv_color_hex(0x606060), 0);
        lv_obj_set_style_text_letter_space(side, 3, 0);
        lv_label_set_text(side, i == 0 ? "LEFT" : "RIGHT");
        lv_obj_align(side, LV_ALIGN_BOTTOM_MID, 0, -4);
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
