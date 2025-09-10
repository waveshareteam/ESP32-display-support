/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "bsp/touch.h"
#include "esp_brookesia.hpp"
#include "bsp_board_extra.h"

#include "app_examples/phone/simple_conf/src/phone_app_simple_conf.hpp"
#include "app_examples/phone/complex_conf/src/phone_app_complex_conf.hpp"
#include "app_examples/phone/squareline/src/phone_app_squareline.hpp"
#include "apps.h"

#include "dark/stylesheet.h"

static const char *TAG = "main";

static void my_rounder_cb(lv_disp_drv_t *disp_drv, lv_area_t *area)
{
    area->x1 = (area->x1 >> 1) << 1;
    area->y1 = (area->y1 >> 1) << 1;
    area->x2 = ((area->x2 >> 1) << 1) + 1;
    area->y2 = ((area->y2 >> 1) << 1) + 1;
}

extern "C" void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(bsp_spiffs_mount());
    ESP_LOGI(TAG, "SPIFFS mount successfully");

    ESP_ERROR_CHECK(bsp_extra_codec_init());

    lv_display_t *disp = bsp_display_start();

    if (disp && disp->driver) {
        disp->driver->rounder_cb = my_rounder_cb;
    }
    // if (disp != NULL)
    // {
    //     bsp_display_rotate(disp, LV_DISPLAY_ROTATION_180);
    // }

    bsp_display_lock(0);

    ESP_Brookesia_Phone *phone = new ESP_Brookesia_Phone(disp);
    assert(phone != nullptr && "Failed to create phone");

    ESP_Brookesia_PhoneStylesheet_t *stylesheet = nullptr;
    stylesheet = new ESP_Brookesia_PhoneStylesheet_t ESP_BROOKESIA_PHONE_DEFAULT_DARK_STYLESHEET();
    ESP_BROOKESIA_CHECK_NULL_EXIT(stylesheet, "Create stylesheet failed");

    if (stylesheet != nullptr)
    {
        ESP_LOGI(TAG, "Using stylesheet (%s)", stylesheet->core.name);
        ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->addStylesheet(stylesheet), "Add stylesheet failed");
        ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->activateStylesheet(stylesheet), "Activate stylesheet failed");
        delete stylesheet;
    }

    ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->setTouchDevice(bsp_display_get_input_dev()), "Set touch device failed");
    phone->registerLvLockCallback((ESP_Brookesia_LvLockCallback_t)(bsp_display_lock), 0);
    phone->registerLvUnlockCallback((ESP_Brookesia_LvUnlockCallback_t)(bsp_display_unlock));
    ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->begin(), "Begin failed");

    Calculator *calculator = new Calculator();
    assert(calculator != nullptr && "Failed to create calculator");
    assert((phone->installApp(calculator) >= 0) && "Failed to begin calculator");

    MusicPlayer *music_player = new MusicPlayer();
    assert(music_player != nullptr && "Failed to create music_player");
    assert((phone->installApp(music_player) >= 0) && "Failed to begin music_player");

    Drawpanel *drawpanel = new Drawpanel();
    assert(drawpanel != nullptr && "Failed to create drawpanel");
    assert((phone->installApp(drawpanel) >= 0) && "Failed to begin drawpanel");

    PhoneAppSimpleConf *app_simple_conf = new PhoneAppSimpleConf();
    ESP_BROOKESIA_CHECK_NULL_EXIT(app_simple_conf, "Create app simple conf failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT((phone->installApp(app_simple_conf) >= 0), "Install app simple conf failed");

    // PhoneAppComplexConf *app_complex_conf = new PhoneAppComplexConf();
    // ESP_BROOKESIA_CHECK_NULL_EXIT(app_complex_conf, "Create app complex conf failed");
    // ESP_BROOKESIA_CHECK_FALSE_EXIT((phone->installApp(app_complex_conf) >= 0), "Install app complex conf failed");

    PhoneAppSquareline *app_squareline = new PhoneAppSquareline();
    ESP_BROOKESIA_CHECK_NULL_EXIT(app_squareline, "Create app squareline failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT((phone->installApp(app_squareline) >= 0), "Install app squareline failed");

    bsp_display_unlock();
}
