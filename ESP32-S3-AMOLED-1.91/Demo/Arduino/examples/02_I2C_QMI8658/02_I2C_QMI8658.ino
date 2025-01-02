/**
 ******************************************************************************
 * @file    I2C_QMI8658C.ino
 * @author   Rongmin Wu
 * @version  V1.0
 * @date     2024-10-30
 * @brief    QMI8658 sensor setup experiment
 * @license  MIT
 * @copyright Copyright (c) 2024, Waveshare
 ******************************************************************************
 * 
 * Experiment Objective: Learn how to set up and run tasks related to the QMI8658 sensor.
 *
 * Hardware Resources and Pin Assignment: 
 * 1. QMI8658 Sensor Interface --> ESP32 Module
 *    Pins used: Connected via I2C.
 *
 * Experiment Phenomenon:
 * 1. Serial communication is initialized.
 * 2. The I2C master is initialized.
 * 3. A task for handling the QMI8658 sensor is created.
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
#include "qmi8658c.h"
#include "I2c_bsp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
void setup()
{
  Serial.begin(115200);
  I2C_master_Init();
  xTaskCreate(qmi8658c_example,"PCF85063_example", 3000, NULL , 2, NULL);
}
void loop()
{
  
}
