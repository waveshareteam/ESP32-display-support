#include <stdio.h>
#include "user_app.h"

#include "i2c_bsp.h"
#include "qmi8658c.h"

void user_app_init(void);

void user_top_init(void)
{
  I2C_master_Init();    //I2C
  user_app_init();      //example
}
void user_app_init(void)
{
  //vTaskDelay(pdMS_TO_TICKS(5000));
  //xTaskCreate(example_rs485_task, "example_rs485_task", 3000, NULL, 2, &rs485_test);
  //xTaskCreate(example_hasswp_task, "example_hasswp_task", 6000,(void*)(&xmqttMess), 2, &hasswp_test);
  //xTaskCreate(qmi8658c_example, "qmi8658c_example", 3000, NULL, 2, NULL);
  //xTaskCreate(PCF85063_example, "PCF85063_example", 3000, NULL , 2, NULL);
  xTaskCreate(qmi8658c_example, "qmi8658c_example", 3000, NULL , 2, NULL);
  //xTaskCreate(example_can_task_read, "example_can_task_read", 3000, NULL , 2, NULL);
}



