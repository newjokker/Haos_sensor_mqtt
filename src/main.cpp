#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>  // 需要安装 ArduinoJson 库
#include <DHT.h>         // 添加DHT传感器库

Preferences prefs;
WebServer server(80);

#define BOOT_PIN 0
#define BOOT_HOLD_MS 3000
#define DHTPIN  8        // 将温度传感器连接到GPIO 4 (可根据需要修改)
#define DHTTYPE DHT11    // 传感器类型 - 可以是 DHT11 或 DHT22

float lastTemperature = 0.0;
unsigned long lastTemperatureRead = 0;
const unsigned long TEMPERATURE_INTERVAL = 5 * 60 * 1000; // 5分钟 (毫秒)

unsigned long lastAvailabilityReport = 0;
const unsigned long AVAILABILITY_INTERVAL = 300000; // 5分钟

String ssidSaved = "";
String passSaved = "";
bool shouldEnterAP = false;

String generateHADiscoveryConfig();

// 设备信息 - 使用唯一标识
String deviceName = "TempSensor_" + String((uint32_t)ESP.getEfuseMac(), HEX).substring(0, 4);
String entityName = "Temperature Sensor";                // HA 中代表温度传感器
String deviceLocation = "Living Room";      

// MQTT 配置 - 使用唯一Client ID
String mqtt_server = "8.153.160.138";
String mqtt_client_id = "tempsensor_" + String((uint32_t)ESP.getEfuseMac(), HEX);  // MQTT 协议规定：相同的 Client ID 不能同时在线

// AP 配置 - 使用唯一AP名称
String ap_ssid = "ESP32-Temp-" + String((uint32_t)ESP.getEfuseMac(), HEX).substring(0, 4);
String ap_password = "12345678";

// MQTT 主题（动态生成）
String temperature_topic;
String availability_topic;
String ha_config_topic;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

DHT dht(DHTPIN, DHTTYPE); // 创建DHT对象


// =========================
// 工具函数
// =========================
void saveWifiConfig(const String &ssid, const String &pass) {
  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
}

void loadWifiConfig() {
  prefs.begin("wifi", true);
  ssidSaved = prefs.getString("ssid", "");
  passSaved = prefs.getString("pass", "");
  prefs.end();
}

void clearWifiConfig() {
  prefs.begin("wifi", false);
  prefs.clear();
  prefs.end();
}

void saveDeviceConfig(const String &name, const String &description, const String &location) {
  prefs.begin("device", false);
  prefs.putString("name", name);
  prefs.putString("description", description);
  prefs.putString("location", location);
  prefs.end();
}

void loadDeviceConfig() {
  prefs.begin("device", true);
  deviceName = prefs.getString("name", "TempSensor_" + String((uint32_t)ESP.getEfuseMac(), HEX).substring(0, 4));
  entityName = prefs.getString("description", "Temperature Sensor");
  deviceLocation = prefs.getString("location", "Unknown Location");
  prefs.end();
}

// 获取唯一标识符
String getUniqueID() {
  return "tempsensor_" + String((uint32_t)ESP.getEfuseMac(), HEX);
}

String getShortID() {
  return String((uint32_t)ESP.getEfuseMac(), HEX).substring(0, 6);
}

// 初始化MQTT主题
void setupTopics() {
  String uid = getShortID();
  temperature_topic = "homeassistant/sensor/temperature_" + uid + "/temperature";
  availability_topic = "homeassistant/sensor/temperature_" + uid + "/availability";
  ha_config_topic = "homeassistant/sensor/temperature_" + uid + "/config";
  
  Serial.println("MQTT主题配置:");
  Serial.println("  温度主题: " + temperature_topic);
  Serial.println("  可用性主题: " + availability_topic);
  Serial.println("  配置主题: " + ha_config_topic);
}

// =========================
// BOOT 长按检测
// =========================
bool checkBootLongPress() {
  if (digitalRead(BOOT_PIN) == LOW) {
    unsigned long start = millis();
    while (digitalRead(BOOT_PIN) == LOW) {
      if (millis() - start >= BOOT_HOLD_MS) {
        return true;
      }
      delay(20);
    }
  }
  return false;
}

// =========================
// AP 配网模式
// =========================
void startAPMode() {
  Serial.println("启动AP模式: " + ap_ssid);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());
  Serial.print("AP IP地址: ");
  Serial.println(WiFi.softAPIP());

  // 设置UTF-8编码支持中文
  String htmlPage = 
    "<!DOCTYPE html><html><head>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>ESP32 温度传感器配置 - " + getShortID() + "</title>"
    "<style>"
    "body{font-family:'Microsoft YaHei',Arial,sans-serif;background:#f2f2f2;text-align:center;padding-top:60px;}"
    ".card{background:white;margin:0 auto;padding:25px;border-radius:10px;max-width:350px;"
    "box-shadow:0 0 10px rgba(0,0,0,0.15);}"
    "h2{color:#333;margin-bottom:20px;}"
    "input{width:100%;padding:12px;margin-top:15px;border-radius:5px;border:1px solid #ccc;"
    "box-sizing:border-box;font-size:14px;}"
    "button{margin-top:20px;padding:12px;width:100%;background:#007BFF;color:white;"
    "border:none;border-radius:5px;font-size:16px;cursor:pointer;}"
    "button:hover{background:#0056b3;}"
    ".info{color:#666;font-size:12px;margin-top:10px;}"
    "</style></head><body>"
    "<div class='card'>"
    "<h2>ESP32 温度传感器配置</h2>"
    "<p style='color:#666;font-size:14px;'>设备ID: " + getShortID() + "</p>"
    "<form method='POST' action='/save'>"
    "<input name='ssid' placeholder='WiFi 名称 (SSID)' required>"
    "<input name='pass' placeholder='WiFi 密码' required>"
    "<input name='name' placeholder='设备名称' value='" + deviceName + "'>"
    "<input name='location' placeholder='设备位置' value='" + deviceLocation + "'>"
    "<input name='description' placeholder='实体名称' value='" + entityName + "'>"
    "<button type='submit'>保存并重启</button>"
    "<p class='info'>设备MAC地址: " + WiFi.macAddress() + "</p>"
    "<p class='info'>设备唯一ID: " + getUniqueID() + "</p>"
    "</form></div></body></html>";

  server.on("/", HTTP_GET, [htmlPage]() {
    server.send(200, "text/html; charset=UTF-8", htmlPage);
  });

  server.on("/save", HTTP_POST, []() {
    // 设置UTF-8编码
    server.sendHeader("Content-Type", "text/html; charset=UTF-8");
    
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    String name = server.arg("name");
    String description = server.arg("description");
    String location = server.arg("location");
    
    if (ssid.length() > 0 && pass.length() > 0) {
      saveWifiConfig(ssid, pass);
      saveDeviceConfig(name, description, location);
      
      String successPage = 
        "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
        "<title>配置成功</title><style>"
        "body{font-family:'Microsoft YaHei',Arial,sans-serif;text-align:center;padding-top:100px;}"
        "</style></head><body>"
        "<h2>✅ 配置保存成功!</h2>"
        "<p>设备即将重启并连接WiFi...</p>"
        "<p>SSID: " + ssid + "</p>"
        "<p>设备名: " + name + "</p>"
        "<p>描述: " + description + "</p>"
        "<p>位置: " + location + "</p>"
        "</body></html>";
      
      server.send(200, "text/html; charset=UTF-8", successPage);
      Serial.println("配置保存成功，准备重启...");
      delay(3000);
      ESP.restart();
    } else {
      String errorPage = 
        "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
        "<title>错误</title><style>"
        "body{font-family:'Microsoft YaHei',Arial,sans-serif;text-align:center;padding-top:100px;}"
        "</style></head><body>"
        "<h2>❌ 错误!</h2>"
        "<p>WiFi名称和密码不能为空</p>"
        "<p><a href='/'>返回重新配置</a></p>"
        "</body></html>";
      
      server.send(400, "text/html; charset=UTF-8", errorPage);
    }
  });

  server.begin();
  Serial.println("HTTP服务器已启动");
}

// =========================
// MQTT 回调
// =========================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  msg.trim();
  
  Serial.printf("MQTT收到消息 [%s]: %s\n", topic, msg.c_str());
  // 温度传感器通常不需要处理命令
}

// =========================
// MQTT 重连和 HA 自动发现
// =========================
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("尝试连接MQTT服务器...");
    
    if (mqttClient.connect(mqtt_client_id.c_str(), availability_topic.c_str(), 0, true, "offline")) {
      Serial.println("MQTT连接成功!");
      
      // 发布在线状态
      mqttClient.publish(availability_topic.c_str(), "online", true);
      
      // 发布 Home Assistant 自动发现配置
      String configPayload = generateHADiscoveryConfig();

      if (mqttClient.publish(ha_config_topic.c_str(), configPayload.c_str(), true)) {
        Serial.println("Home Assistant自动发现配置发布成功!");
      } else {
        Serial.println("Home Assistant自动发现配置发布失败!");
      }
      
    } else {
      Serial.print("MQTT连接失败, 错误代码=");
      Serial.print(mqttClient.state());
      Serial.println("，5秒后重试...");
      delay(5000);
    }
  }
}

// =========================
// WiFi 连接
// =========================
void connectWiFi() {
  Serial.printf("正在连接WiFi: %s\n", ssidSaved.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssidSaved.c_str(), passSaved.c_str());

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime > 30000) { // 30秒超时
      Serial.println("WiFi连接超时，进入AP模式");
      clearWifiConfig();
      ESP.restart();
      return;
    }
    
    if (checkBootLongPress()) {
      Serial.println("检测到BOOT按钮长按，进入AP模式");
      clearWifiConfig();
      ESP.restart();
      return;
    }
    
    Serial.print(".");
    delay(1000);
  }
  
  Serial.println();
  Serial.print("✅ WiFi连接成功! IP地址: ");
  Serial.println(WiFi.localIP());
}

// =========================
// 读取并发布温度数据
// =========================
void readAndPublishTemperature() {
  // 读取温度
  float temperature = dht.readTemperature();
  
  // 检查读取是否成功
  if (isnan(temperature)) {
    Serial.println("❌ 无法从DHT传感器读取温度数据!");
    return;
  }
  
  lastTemperature = temperature;
  lastTemperatureRead = millis();
  
  // 发布温度数据
  char tempMsg[10];
  dtostrf(temperature, 4, 2, tempMsg);
  mqttClient.publish(temperature_topic.c_str(), tempMsg, true);
  Serial.print("🌡️ 温度数据已发布: ");
  Serial.print(temperature);
  Serial.println("°C");
  
  Serial.println("✅ 温度数据上传完成");
}

// =========================
// 生成HA自动发现配置
// =========================
String generateHADiscoveryConfig() {
  String uid = getShortID();
  
  // 使用 ArduinoJson 库生成正确的 JSON
  DynamicJsonDocument doc(1024);
  
  // 基本配置
  doc["name"] = entityName;                                // 实体名称
  doc["unique_id"] = "temperature_" + uid;                 // 唯一ID
  doc["state_topic"] = temperature_topic;                  // 状态主题
  doc["availability_topic"] = availability_topic;          // 可用性主题
  doc["payload_available"] = "online";
  doc["payload_not_available"] = "offline";
  doc["device_class"] = "temperature";                     // 设备类别为温度
  doc["unit_of_measurement"] = "°C";                       // 单位为摄氏度
  doc["value_template"] = "{{ value_json.temperature }}";  // 模板（如果使用JSON格式）
  doc["retain"] = true;
  doc["friendly_name"] = "温湿度传感器的 friend 名字";
  
  // 设备信息
  JsonObject device = doc.createNestedObject("device");
  device["identifiers"][0] = "temperature_" + uid;
  device["name"] = deviceName;                              // 设备名称
  device["manufacturer"] = "selfmade sensor";
  device["model"] = "DHT" + String(DHTTYPE == DHT11 ? "11" : "22");
  device["sw_version"] = "1.0";
  
  // 添加位置信息（可选）
  if (deviceLocation != "Unknown Location") {
    device["suggested_area"] = deviceLocation;  // 用于Home Assistant的区域识别
  }
  
  String configPayload;
  serializeJson(doc, configPayload);
  
  Serial.println("生成的HA自动发现配置:");
  Serial.println(configPayload);
  
  return configPayload;
}

// =========================
// 主流程
// =========================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32 MQTT 温度传感器启动 ===");
  Serial.println("设备唯一ID: " + getUniqueID());
  Serial.println("短ID: " + getShortID());
  
  pinMode(BOOT_PIN, INPUT_PULLUP);
  
  // 初始化DHT传感器
  dht.begin();
  Serial.println("✅ DHT" + String(DHTTYPE == DHT11 ? "11" : "22") + " 传感器初始化完成");

  // 加载配置
  loadWifiConfig();
  loadDeviceConfig();
  
  Serial.println("设备信息:");
  Serial.println("  MAC地址: " + WiFi.macAddress());
  Serial.println("  设备名: " + deviceName);
  Serial.println("  实体名: " + entityName);
  Serial.println("  位置: " + deviceLocation);
  Serial.println("  MQTT Client ID: " + mqtt_client_id);
  Serial.println("  AP名称: " + ap_ssid);

  // 检查是否进入配网模式
  if (checkBootLongPress() || ssidSaved == "" || passSaved == "") {
    Serial.println("进入AP配网模式");
    shouldEnterAP = true;
    startAPMode();
  } else {
    connectWiFi();
    
    // 初始化MQTT主题
    setupTopics();
    
    // 设置MQTT
    mqttClient.setServer(mqtt_server.c_str(), 1883);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setBufferSize(2048); // 增加缓冲区大小
    
    reconnectMQTT();
  }
}

void loop() {
  if (shouldEnterAP) {
    server.handleClient();
    return;
  }

  // 处理MQTT
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();

  // 每5分钟读取并发布一次温度数据
  static unsigned long lastSensorTime = -TEMPERATURE_INTERVAL - 10;  // 确保第一次就能读取数据
  if (millis() - lastSensorTime > TEMPERATURE_INTERVAL) {
    readAndPublishTemperature();
    lastSensorTime = millis();
  }

  // 定期上报在线状态
  if (millis() - lastAvailabilityReport > AVAILABILITY_INTERVAL) {
    mqttClient.publish(availability_topic.c_str(), "online", true);
    lastAvailabilityReport = millis();
    Serial.println("上报在线状态");
  }

  // 检查BOOT长按
  if (checkBootLongPress()) {
    Serial.println("检测到BOOT按钮长按，清除配置并重启");
    clearWifiConfig();
    delay(1000);
    ESP.restart();
  }
}