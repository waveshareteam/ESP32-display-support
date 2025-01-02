idf_component_register(
  SRCS "user_bsp.c"
  REQUIRES  lvgl esp_wifi_bsp adc_bsp qmi8658c ui_bsp driver i2c_bsp sd_card_bsp
  INCLUDE_DIRS ".")
