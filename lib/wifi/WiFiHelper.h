#ifndef WIFI_HELPER_H
#define WIFI_HELPER_H

#include <WiFi.h>

class WiFiHelper {
public:
    WiFiHelper(const char* ssid, const char* password);
    void connect();

private:
    const char* ssid;
    const char* password;
};

#endif
