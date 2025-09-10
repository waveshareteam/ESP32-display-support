#pragma once

#include "lvgl.h"
#include "esp_brookesia.hpp"

class Drawpanel: public ESP_Brookesia_PhoneApp
{
public:
    Drawpanel();
    ~Drawpanel();

    bool run(void);
    bool back(void);
    bool close(void);

    bool init(void) override;

private:
    static void touch_event_cb(lv_event_t *e);
    lv_point_t prev_point;
};
