#ifndef MQTTHelper_H
#define MQTTHelper_H

#include <WiFi.h>
#include <PubSubClient.h>

class MqttHelper {
public:
    MqttHelper(const char* server, int port, const char* clientId);

    void begin();
    void loop();
    bool connect();
    bool isConnected();

    void setCallback(MQTT_CALLBACK_SIGNATURE);
    bool subscribe(const char* topic);
    bool publish(const char* topic, const char* message);

private:
    WiFiClient espClient;
    PubSubClient mqttClient;
    const char* server;
    int port;
    const char* clientId;
};

#endif
