#include "layer_status.h"

#include <zmk/display.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/keymap.h>

#include <fonts.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct layer_status_state {
    uint8_t index;
};

static void layer_status_update(lv_obj_t *container, struct layer_status_state state) {
    lv_obj_t *label = lv_obj_get_child(container, 0);

    if (state.index == 0) {
        lv_obj_fade_out(container, 200, 0);
    } else {
        const char *name =
            zmk_keymap_layer_name(zmk_keymap_layer_index_to_id(state.index));
        if (name && *name) {
            lv_label_set_text(label, name);
        } else {
            lv_label_set_text_fmt(label, "LAYER %d", state.index);
        }
        lv_obj_fade_in(container, 200, 0);
    }
}

static void layer_status_update_cb(struct layer_status_state state) {
    struct zmk_widget_layer_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        layer_status_update(widget->obj, state);
    }
}

static struct layer_status_state layer_status_get_state(const zmk_event_t *eh) {
    uint8_t index = zmk_keymap_highest_layer_active();
    return (struct layer_status_state){
        .index = index,
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_status, struct layer_status_state,
                            layer_status_update_cb, layer_status_get_state)
ZMK_SUBSCRIPTION(widget_layer_status, zmk_layer_state_changed);

int zmk_widget_layer_status_init(struct zmk_widget_layer_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(widget->obj, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(widget->obj, 255, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);

    /* Start hidden (default layer shows battery view) */
    lv_obj_set_style_opa(widget->obj, 0, 0);

    /* Layer name label */
    lv_obj_t *label = lv_label_create(widget->obj);
    lv_obj_set_style_text_font(label, &FRAC_Bold_48, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(label, "");

    sys_slist_append(&widgets, &widget->node);

    widget_layer_status_init();
    return 0;
}

lv_obj_t *zmk_widget_layer_status_obj(struct zmk_widget_layer_status *widget) {
    return widget->obj;
}
