#include <WiFi.h>

const char* ssid     = "bsp_esp_demo";//
const char* password = "waveshare";//      

void setup() {
  Serial.begin(115200); 
  WiFi.softAP(ssid,password);
  delay(1000);
}

void loop() {                           
}
