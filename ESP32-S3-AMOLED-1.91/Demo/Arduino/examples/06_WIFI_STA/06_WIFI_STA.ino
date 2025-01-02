/**
 ******************************************************************************
 * @file     WIFI_STA.ino
 * @author   Rongmin Wu
 * @version  V1.0
 * @date     2024-10-30
 * @brief    ESP32 WiFi setup experiment
 * @license  MIT
 * @copyright Copyright (c) 2024, Waveshare
 ******************************************************************************
 * 
 * Experiment Objective: Learn how to initialize the WiFi on an ESP32.
 *
 * Hardware Resources and Pin Assignment: 
 * 1. ESP32 WiFi Module.
 *
 * Experiment Phenomenon:
 * 1. Serial communication is initialized.
 * 2. After a delay, the WiFi is initialized.
 * 
 * Notes:
 * None
 * 
 ******************************************************************************
 * 
 * Development Platform: ESP32
 * Support Forum: service.waveshare.com
 * Company Website: www.waveshare.com
 *
 ******************************************************************************
 */
#include "espwifi.h"
void setup() 
{
  Serial.begin(115200);
  delay(3000);
  wifi_init();
}

void loop()
{
  
}

