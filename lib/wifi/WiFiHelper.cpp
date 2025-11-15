#include "WiFiHelper.h"
#include <Arduino.h>

WiFiHelper::WiFiHelper(const char* ssid, const char* password)
    : ssid(ssid), password(password) {}

void WiFiHelper::connect() {
    Serial.print("🔌 正在连接 WiFi: ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\n✅ WiFi 连接成功！");
    Serial.print("🌐 IP 地址: ");
    Serial.println(WiFi.localIP());
}
