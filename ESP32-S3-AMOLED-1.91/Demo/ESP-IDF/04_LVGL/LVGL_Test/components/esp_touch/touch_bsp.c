#include <stdio.h>
#include "touch_bsp.h"
#include "i2c_bsp.h"
#define EXAMPLE_LCD_H_RES  536
#define EXAMPLE_LCD_V_RES  240
#define I2C_ADDR_FT3168 0x38

/*
void Touch_Init(void)
{
  const i2c_config_t i2c_conf = 
  {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = EXAMPLE_PIN_NUM_TOUCH_SDA,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_io_num = EXAMPLE_PIN_NUM_TOUCH_SCL,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 300 * 1000,
  };
  ESP_ERROR_CHECK(i2c_param_config(TOUCH_HOST, &i2c_conf));
  ESP_ERROR_CHECK(i2c_driver_install(TOUCH_HOST, i2c_conf.mode, 0, 0, 0));

  uint8_t data = 0x00;
  I2C_writr_buff(I2C_ADDR_FT3168,0x00,&data,1); //Switch to normal mode

}
*/
void touch_Init(void)
{
  uint8_t data = 0x00;
  I2C_writr_buff(I2C_ADDR_FT3168,0x00,&data,1); //Switch to normal mode
}
uint8_t getTouch(uint16_t *x,uint16_t *y)
{
  uint8_t data;
  uint8_t buf[4];
  I2C_read_buff(I2C_ADDR_FT3168,0x02,&data,1);
  if(data)
  {
    I2C_read_buff(I2C_ADDR_FT3168,0x03,buf,4);
    *y = (((uint16_t)buf[0] & 0x0f)<<8) | (uint16_t)buf[1];
    *x = (((uint16_t)buf[2] & 0x0f)<<8) | (uint16_t)buf[3];
    if(*x > EXAMPLE_LCD_H_RES)
    *x = EXAMPLE_LCD_H_RES;
    if(*y > EXAMPLE_LCD_V_RES)
    *y = EXAMPLE_LCD_V_RES;
    *y = EXAMPLE_LCD_V_RES - *y;
    return 1;
  }
  return 0;
}