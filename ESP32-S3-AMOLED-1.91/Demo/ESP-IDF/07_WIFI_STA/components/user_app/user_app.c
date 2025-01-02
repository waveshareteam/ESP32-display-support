#include <stdio.h>
#include "user_app.h"

#include "esp_wifi_bsp.h"

void user_app_init(void);

void user_top_init(void)
{
  printf("wifi-example run \n");
  espwifi_Init();
}
void user_app_init(void)
{
  //vTaskDelay(pdMS_TO_TICKS(5000));
  //xTaskCreate(example_rs485_task, "example_rs485_task", 3000, NULL, 2, &rs485_test);
  //xTaskCreate(example_hasswp_task, "example_hasswp_task", 6000,(void*)(&xmqttMess), 2, &hasswp_test);
  //xTaskCreate(qmi8658c_example, "qmi8658c_example", 3000, NULL, 2, NULL);
  //xTaskCreate(adc_example, "adc_example", 3000, NULL , 2, NULL);
  //xTaskCreate(adc_example, "adc_example", 3000, NULL , 2, NULL);
  //xTaskCreate(example_can_task_read, "example_can_task_read", 3000, NULL , 2, NULL);
}



