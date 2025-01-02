#include "espwifi.h"
/**************WIFI Declarations***********************/

const char* ssid1 = "bsp_esp_demo";
const char* password1 = "waveshare";

/* Static IP */
IPAddress staticIP(192,168,3,220);  // ESP32's IP address
IPAddress gateway(192,168,3,1);     // Gateway IP address
IPAddress subnet(255,255,255,0);    // Subnet mask
IPAddress dnsIP(192,168,3,1);       // DNS server IP

#define ssid ssid1
#define password password1

// WiFi connection
void wifi_init(void)
{
  int WIFIBUFF=0;
  WiFi.mode(WIFI_STA); // Set ESP32 mode. Returns 1 if successful. Station mode (client mode)
  // WiFi.config(staticIP, gateway, subnet , dnsIP); // Set static IP address
  /******************WIFI Connection********************/
  WiFi.begin(ssid, password); // Connect to WiFi
  while (WiFi.status() != WL_CONNECTED) // Start WiFi and connect
  {
    WIFIBUFF++;
    delay(500);
    if (WIFIBUFF > 20)
    {
      break; 
    }
  }
  if (WIFIBUFF < 20)
  {
    Serial.print("IP: ");
    Serial.print(WiFi.localIP());
  }
  else
  {
    Serial.println("Failed to connect to WiFi");
  }
  // WiFi.setAutoConnect(1); // Automatically reconnect to WiFi on startup. Returns 1 if successful.
  WiFi.setAutoReconnect(1); // Enable auto reconnect on WiFi disconnection
}

// Check if connected to access point. Returns true if connected, false if not
bool Wifi_isConned(void)
{
  return WiFi.isConnected();
}

// Connect to WiFi
void Wifi_onConned(void)
{
  WiFi.begin(ssid, password); // WiFi connection
}
