#ifndef SD_CARD_BSP_H
#define SD_CARD_BSP_H
#include "esp_err.h"
void SD_card_Init(void);
float sd_card_get_value(void);

esp_err_t s_example_write_file(const char *path, char *data);
esp_err_t s_example_read_file(const char *path,uint8_t *pxbuf,uint32_t *outLen);

#endif