#include "Drawpanel.hpp"
#include "lvgl.h"

LV_IMG_DECLARE(img_app_drawpanel);

Drawpanel::Drawpanel() : ESP_Brookesia_PhoneApp("Drawpanel", &img_app_drawpanel, true)
{
}

Drawpanel::~Drawpanel()
{
}

bool Drawpanel::run(void)
{
    lv_area_t area = getVisualArea();
    int _width = area.x2 - area.x1;
    int _height = area.y2 - area.y1;

    lv_obj_t *panel_obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(panel_obj, _width, _height);
    lv_obj_align(panel_obj, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_add_event_cb(panel_obj, touch_event_cb, LV_EVENT_PRESSING, this);

    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLL_CHAIN_VER);

    return true;
}

bool Drawpanel::back(void)
{
    notifyCoreClosed();

    return true;
}

bool Drawpanel::close(void)
{
    return true;
}

bool Drawpanel::init(void)
{
    return true;
}

void Drawpanel::touch_event_cb(lv_event_t *e)
{
    Drawpanel *app = (Drawpanel *)lv_event_get_user_data(e);
    lv_indev_t *indev = lv_indev_get_act();
    lv_point_t point;

    lv_indev_get_point(indev, &point);

    lv_obj_t *dot = lv_obj_create(lv_scr_act());
    lv_obj_set_size(dot, 10, 10);                                // 设置圆点的大小
    lv_obj_set_pos(dot, point.x, point.y - 40);                       // 将圆点中心移到触摸点
    lv_obj_set_style_bg_color(dot, lv_color_make(255, 0, 0), 0); // 设置圆点颜色为红色
    lv_obj_set_style_radius(dot, 5, 0);                          // 设置圆角半径，使其为圆形
}
