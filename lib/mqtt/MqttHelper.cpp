#include "MqttHelper.h"
#include <Arduino.h>

MqttHelper::MqttHelper(const char* server, int port, const char* clientId)
    : mqttClient(espClient), server(server), port(port), clientId(clientId) {
}

void MqttHelper::begin() {
    mqttClient.setServer(server, port);
}

void MqttHelper::loop() {
    mqttClient.loop();
}

void MqttHelper::setCallback(MQTT_CALLBACK_SIGNATURE) {
    mqttClient.setCallback(callback);
}

bool MqttHelper::connect() {
    if (mqttClient.connected()) return true;

    // Serial.print("🔗 尝试连接 MQTT Broker...");

    if (mqttClient.connect(clientId)) {
        // Serial.println("✅ MQTT 连接成功！");
        return true;
    } else {
        Serial.print("❌ 失败，错误码 = ");
        Serial.println(mqttClient.state());
        return false;
    }
}

bool MqttHelper::isConnected() {
    return mqttClient.connected();
}

bool MqttHelper::subscribe(const char* topic) {
    if (mqttClient.subscribe(topic)) {
        Serial.print("📡 已订阅主题: ");
        Serial.println(topic);
        return true;
    }
    Serial.print("⚠️ 订阅失败: ");
    Serial.println(topic);
    return false;
}

bool MqttHelper::publish(const char* topic, const char* message) {
    if (mqttClient.publish(topic, message)) {
        Serial.print("📤 已发布消息 | 主题 = ");
        Serial.print(topic);
        Serial.print(" | 内容 = ");
        Serial.println(message);
        return true;
    }
    Serial.println("⚠️ 发布失败");
    return false;
}


/*  使用示例


#include <Arduino.h>
#include "WiFi.h"
#include "MqttHelper.h"
#include "WiFiHelper.h"

// ================= 配置信息 =================
const char* WIFI_SSID     = "Saturn-Guest-2.4g";
const char* WIFI_PASSWORD = "Tuxingkeji-0918";

const char* MQTT_SERVER   = "8.153.160.138";
const int   MQTT_PORT     = 1883;
const char* MQTT_CLIENT_ID = "核心板";              // 这个不能重复，一样的名字后面的会把前面的顶掉

// 工具类
MqttHelper mqttHelper(MQTT_SERVER, MQTT_PORT, MQTT_CLIENT_ID);
WiFiHelper wifiHelper(WIFI_SSID, WIFI_PASSWORD);


void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("📩 收到消息 | 主题 = ");
    Serial.print(topic);
    Serial.print(" | 内容 = ");

    String msg;
    for (unsigned int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }
    Serial.println(msg);
}

void setup() {
    Serial.begin(115200);

    wifiHelper.connect();
    mqttHelper.begin();
    mqttHelper.setCallback(mqttCallback);
}

void loop() {
    
  // 如果 MQTT 掉线，尝试重连
    if (!mqttHelper.isConnected()) {
         Serial.print("❌");
        if (mqttHelper.connect()) {
            mqttHelper.subscribe("test/topic");   // 自动重新订阅
        }
    }

    mqttHelper.loop();   // 必须常驻执行

    // 每 3 秒发布一次消息
    static unsigned long lastTime = 0;
    if (millis() - lastTime > 3000) {
        mqttHelper.publish("test2/topic", "Hello from 核心板子!");
        lastTime = millis();
    }
}


 */
