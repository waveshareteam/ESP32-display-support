#ifndef TOUCH_BSP_H
#define TOUCH_BSP_H

#ifdef __cplusplus
extern "C" {
#endif
void touch_Init(void);
uint8_t getTouch(uint16_t *x,uint16_t *y);
#ifdef __cplusplus
}
#endif

#endif
