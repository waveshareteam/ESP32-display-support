#include <stdio.h>
#include "adc_bsp.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


static adc_oneshot_unit_handle_t adc1_handle;
void adc_bsp_init(void)
{
  adc_oneshot_unit_init_cfg_t init_config1 = {
    .unit_id = ADC_UNIT_1, //ADC1
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
  adc_oneshot_chan_cfg_t config = {
    .bitwidth = ADC_BITWIDTH_DEFAULT,
    .atten = ADC_ATTEN_DB_12,//ADC_ATTEN_DB_12,         //    1.1          ADC_ATTEN_DB_12:3.3
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &config));
}
void adc_get_value(float *value)
{
  int adcdata;
  esp_err_t err;
  err = adc_oneshot_read(adc1_handle,ADC_CHANNEL_0,&adcdata);
  if(err == ESP_OK)
  {
    *value = ((float)adcdata * 3.3/4096) * 2;
    //*(value+1) = adcdata;
  }
  else
  {
    *value = 0;
    //*(value+1) = 0;
  }
}
/*test demo*/
void adc_example(void* parmeter)
{
  adc_bsp_init();
  int adcdata;
  for(;;)
  {
    ESP_ERROR_CHECK_WITHOUT_ABORT(adc_oneshot_read(adc1_handle,ADC_CHANNEL_1,&adcdata));
    printf("adc value:%d,system voltage:%f\n",adcdata,((float)adcdata * 3.3/4096) * 3);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
