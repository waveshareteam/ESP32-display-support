#include "user_bsp.h"
#include "qmi8658c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "adc_bsp.h"
#include "esp_wifi_bsp.h"
#include "driver/gpio.h"
#include "i2c_bsp.h"
void user_example(void *udata);
void esp_wifi_scan_w(void *arg);
void color_user(void *arg);
static void Wifi_event_handler (lv_event_t *e);
void lv_clear_list(lv_ui *obj);
TaskHandle_t pxWIFIreadTask;

void gpio_bitt(void)
{
  gpio_config_t gpio_conf = {};

  gpio_conf.intr_type = GPIO_INTR_DISABLE;
  gpio_conf.mode = GPIO_MODE_INPUT;
  gpio_conf.pin_bit_mask = (0x01<<0); //GPIO0
  gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;

  ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));

}
void user_bitt(void *pro)
{
  lv_ui *ui = (lv_ui *)pro;
  uint8_t i = 0;
  uint8_t win_for = 0,win_for_to = 0;
  for(;;)
  {
    if(gpio_get_level(0) == 0)
    {
      vTaskDelay(pdMS_TO_TICKS(10));
      while (gpio_get_level(0) == 0)
      {
        vTaskDelay(pdMS_TO_TICKS(10));
      }
      if(!(win_for || win_for_to))
      i++;
      if(i>3)
      i = 0;
      if(!(win_for || win_for_to))
      lv_tabview_set_act(ui->screen_tabview_1,i,LV_ANIM_OFF);
      //vTaskDelay(pdMS_TO_TICKS(10));lv_slider_set_value(slider, target_value, LV_ANIM_ON);
      if(win_for)
      {
        win_for = 0;
        lv_event_send(ui->screen_btn_1, LV_EVENT_CLICKED, NULL);
      }
      else if(win_for_to)
      {
        win_for_to = 0;
        lv_event_send(ui->screen_btn_2, LV_EVENT_CLICKED, NULL);
      }
      else if(i == 2)
      {
        win_for = 1;
      }
      else if(i == 3)
      {
        win_for_to = 1;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void user_bsp_Init(lv_ui *ui)
{
  //lv_tabview_set_act(ui->screen_tabview_1,1,LV_ANIM_OFF);
  //I2C_master_Init();
  //lv_obj_clear_flag(ui->screen_carousel_1,LV_OBJ_FLAG_SCROLLABLE); //先不能滑动
  lv_obj_add_flag(ui->screen_img_1, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui->screen_img_2, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui->screen_img_3, LV_OBJ_FLAG_HIDDEN);
  gpio_bitt();
  espwifi_Init();
  lv_clear_list(ui);
  lv_obj_add_event_cb(ui->screen_btn_2, Wifi_event_handler, LV_EVENT_ALL, ui);
  xTaskCreate(esp_wifi_scan_w, "esp_wifi_scan_w", 3000, ui, 2, &pxWIFIreadTask);
  xTaskCreate(user_example, "LVGL", 5000, ui, 2, NULL);
  xTaskCreate(user_bitt, "user_bitt", 2000, ui, 2, NULL);
  xTaskCreate(color_user, "color_user", 2000, ui, 2, NULL); //切换颜色
}
void user_example(void *udata)
{
  lv_ui *obj = (lv_ui *)udata;
  char qmi_values[30] = {0};
  char adc_values[15] = {0};
  float acc[3] = {0};
  float gyro[3] = {0};
  float adc_value;
  float Temp = 0;
  uint32_t stimes = 0;
  uint32_t qmi_test = 0;
  uint32_t adc_test = 0;
  qmi8658_init();
  adc_bsp_init();
  for(;;)
  {
    if(stimes - qmi_test > 1)      //2s
    {
      qmi_test = stimes;
      qmi8658_read_xyz(acc,gyro);
      sprintf(qmi_values,"ax:%.2f ay:%.2f az:%.2f",acc[0],acc[1],acc[2]);
      lv_label_set_text(obj->screen_label_10, qmi_values);
      Temp = qmi8658_readTemp();
      sprintf(qmi_values,"%.2f",Temp);
      lv_label_set_text(obj->screen_label_6, qmi_values);
    }
    if(stimes - adc_test > 1) //2s
    {
      adc_test = stimes;
      adc_get_value(&adc_value);
      if(adc_value)
      {
        sprintf(adc_values,"%.2fV",adc_value);
        lv_label_set_text(obj->screen_label_8, adc_values);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
    stimes++;
  }
}

static void Wifi_event_handler (lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  switch (code) 
	{
		case LV_EVENT_CLICKED:
		{
			xTaskNotifyGive(pxWIFIreadTask);
			break;
		}
  	default:
  	  break;
  }
}
void lv_clear_list(lv_ui *obj) 
{
	for(signed char i = 9; i>=0; i--)
	{
		lv_obj_t *imte = lv_obj_get_child(obj->screen_list_1,i);
		lv_obj_add_flag(imte,LV_OBJ_FLAG_HIDDEN);
		vTaskDelay(pdMS_TO_TICKS(50));
	}
}
void esp_wifi_scan_w(void *arg)
{
  lv_ui *wifi_obj = (lv_ui *)arg;
	static wifi_ap_record_t recdata;
  static uint16_t rec = 0;
	static const char *imgbox = NULL;
	static lv_obj_t *imte;
	static lv_obj_t *label;
  for(;;)
  {
    ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
		lv_clear_list(wifi_obj);
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_scan_start(NULL,true));               //扫描可用AP
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_scan_get_ap_num(&rec));
    if(rec != 0)
    {
			if(rec > 9)
			{
				rec = 10;
				for(uint8_t i = 0; i<rec; i++)
      	{
      	  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_scan_get_ap_record(&recdata));
					imgbox = (const char*)(&(recdata.ssid[0]));
					if (imgbox == NULL)
					break;
					imte = lv_obj_get_child(wifi_obj->screen_list_1,i);
					if (imte != NULL)
					{
						label = lv_obj_get_child(imte,1);
						if(label != NULL){
    					lv_label_set_text(label,imgbox);
							lv_obj_clear_flag(imte,LV_OBJ_FLAG_HIDDEN);    //显示
						}
					}
					imgbox = NULL;
					imte = NULL;
					label = NULL;
					vTaskDelay(pdMS_TO_TICKS(300));
      	}
				esp_wifi_clear_ap_list();
			}
			else
			{
      	for(uint8_t i = 0; i<rec; i++)
      	{
      	  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_scan_get_ap_record(&recdata));
					imgbox = (const char*)(&(recdata.ssid[0]));
					if (imgbox == NULL)
					break;
					imte = lv_obj_get_child(wifi_obj->screen_list_1,i); //获取子对象
					if (imte != NULL)
					{
						label = lv_obj_get_child(imte,1); //获取子子对象
						if(label != NULL){
    					lv_label_set_text(label,imgbox);
							lv_obj_clear_flag(imte,LV_OBJ_FLAG_HIDDEN);    //显示
						}
					}
					imgbox = NULL;
					imte = NULL;
					label = NULL;
					vTaskDelay(pdMS_TO_TICKS(300));
      	}
			}
    }
  }
}
void color_user(void *arg)
{
  lv_ui *ui = (lv_ui *)arg;
  lv_obj_clear_flag(ui->screen_img_1, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui->screen_img_2, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui->screen_img_3, LV_OBJ_FLAG_HIDDEN);
  vTaskDelay(pdMS_TO_TICKS(1500));
  lv_obj_clear_flag(ui->screen_img_2, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui->screen_img_1, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui->screen_img_3, LV_OBJ_FLAG_HIDDEN);
  vTaskDelay(pdMS_TO_TICKS(1500));
  lv_obj_clear_flag(ui->screen_img_3, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui->screen_img_1, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui->screen_img_2, LV_OBJ_FLAG_HIDDEN);
  vTaskDelay(pdMS_TO_TICKS(1500));
  lv_obj_add_flag(ui->screen_img_1, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui->screen_img_2, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui->screen_img_3, LV_OBJ_FLAG_HIDDEN);
  vTaskDelete(NULL); //删除任务
}
