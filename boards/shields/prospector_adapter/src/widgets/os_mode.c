#include "os_mode.h"

#include <string.h>

#include <zmk/display.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/keymap.h>

#include <fonts.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct os_mode_state {
    bool win_active;
};

static bool detect_win_active(void) {
    for (zmk_keymap_layer_index_t i = 0; i < ZMK_KEYMAP_LAYERS_LEN; i++) {
        zmk_keymap_layer_id_t id = zmk_keymap_layer_index_to_id(i);
        if (id == ZMK_KEYMAP_LAYER_ID_INVAL) continue;
        const char *name = zmk_keymap_layer_name(id);
        if (name && strcmp(name, "WIN") == 0 && zmk_keymap_layer_active(id)) {
            return true;
        }
    }
    return false;
}

static void os_mode_update(lv_obj_t *container, struct os_mode_state state) {
    lv_obj_t *dot = lv_obj_get_child(container, 0);
    lv_obj_t *label = lv_obj_get_child(container, 1);

    if (state.win_active) {
        lv_label_set_text(label, "Windows");
        lv_obj_set_style_bg_color(dot, lv_color_hex(0x3d95ff), LV_PART_MAIN);
    } else {
        lv_label_set_text(label, "macOS");
        lv_obj_set_style_bg_color(dot, lv_color_hex(0xa0a0a0), LV_PART_MAIN);
    }
}

static void os_mode_update_cb(struct os_mode_state state) {
    struct zmk_widget_os_mode *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        os_mode_update(widget->obj, state);
    }
}

static struct os_mode_state os_mode_get_state(const zmk_event_t *eh) {
    return (struct os_mode_state){
        .win_active = detect_win_active(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_os_mode, struct os_mode_state,
                            os_mode_update_cb, os_mode_get_state)
ZMK_SUBSCRIPTION(widget_os_mode, zmk_layer_state_changed);

int zmk_widget_os_mode_init(struct zmk_widget_os_mode *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(widget->obj, 0, LV_PART_MAIN);

    lv_obj_set_flex_flow(widget->obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(widget->obj, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(widget->obj, 8, LV_PART_MAIN);

    /* Child 0: status dot */
    lv_obj_t *dot = lv_obj_create(widget->obj);
    lv_obj_set_size(dot, 8, 8);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(dot, lv_color_hex(0xa0a0a0), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(dot, 255, LV_PART_MAIN);
    lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(dot, 0, LV_PART_MAIN);

    /* Child 1: OS name label */
    lv_obj_t *label = lv_label_create(widget->obj);
    lv_obj_set_style_text_font(label, &SF_Compact_Text_Medium_24, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xa0a0a0), 0);
    lv_obj_set_style_text_letter_space(label, 2, 0);
    lv_label_set_text(label, "macOS");

    sys_slist_append(&widgets, &widget->node);

    widget_os_mode_init();
    return 0;
}

lv_obj_t *zmk_widget_os_mode_obj(struct zmk_widget_os_mode *widget) {
    return widget->obj;
}
