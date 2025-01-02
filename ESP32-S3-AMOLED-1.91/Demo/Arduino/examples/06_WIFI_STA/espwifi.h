#ifndef ESPWIFI_H
#define ESPWIFI_H
#include <Arduino.h>
#include <WiFi.h>

void wifi_init(void);
bool Wifi_isConned(void); // Check if connected to access point. Returns true if connected, false if not.
void Wifi_onConned(void); // Connect to WiFi

#endif
