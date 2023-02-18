#include <Arduino.h>
#include "ESPAsyncWebServer.h"
#include <definitions.h>
#include <FS.h>
#include <storage.hpp>
#include <auth.hpp>
#include <Hash.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Simple_HCSR04.h>
#include <PubSubClient.h>

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
String channelState(int numCh);
String htmlProcess(const String &var);
void deviceInit();
void mqttCallback(char *topic, byte *payload, unsigned int length);
void mqttInit();
void mqttReconnect();

String composeSensorJson();
// Replaces placeholder with button section in your web page
String htmlProcess(const String &var)
{
  Serial.println(var);
  if (var == "DNM")
  {
    return String(DeviceConfig.DeviceName);
  }
  if (var == "APSSID")
  {
    return String(DeviceConfig.ApSSID);
  }
  else if (var == "APPWD")
  {
    return String(DeviceConfig.ApPwd);
  }
  else if (var == "TETPWD")
  {
    return String(DeviceConfig.TetherPwd);
  }
  else if (var == "PUSER")
  {
    return String(DeviceConfig.PortalUser);
  }
  else if (var == "PPWD")
  {
    return String(DeviceConfig.PortalPwd);
  } 
  else if (var == "MAXSTR")
  {
    return String(MAX_CONF_STRING);
  }
  else if (var == "MAXSSID")
  {
    return String(MAX_CONF_SSID_NAME);
  }
  else if (var == "SHAPWD")
  {
    return sha1(DeviceConfig.PortalPwd);
  }
  else if (var == "FWVER")
  {
    return FW_VER;
  }
  else if (var == "PRODNAME")
  {
    return PRODUCT_NAME;
  }  
  else if (var == "SELDHT")
  {
    return DeviceConfig.Device == DeviceSelect::DHT_11 ? "selected" : "";
  }
  else if (var == "SELSOIL")
  {
    return DeviceConfig.Device == SoilMoisture ? "selected" : "";
  }
  else if (var == "SELPIR")
  {
    return DeviceConfig.Device == PIR ? "selected" : "";
  }
  else if (var == "SELLDR")
  {
    return DeviceConfig.Device == LDR ? "selected" : "";
  }
    else if (var == "SELDIST")
  {
    return DeviceConfig.Device == Distance ? "selected" : "";
  }
    else if (var == "SELSOUND")
  {
    return DeviceConfig.Device == Sound ? "selected" : "";
  }
    else if (var == "SELFIRE")
  {
    return DeviceConfig.Device == Fire ? "selected" : "";
  }
  else if (var == "SELRELAY")
  {
    return DeviceConfig.Device == DeviceSelect::Relay ? "selected" : "";
  }
  else if (var == "MQTTSRVR")
  {
    return String(DeviceConfig.MqttServer);
  }
  else if (var == "MQTTPORT")
  {
    return String(DeviceConfig.MqttPort);
  }
  else if (var == "MQTTPORTMIN")
  {
    return "3";
  }
  else if (var == "MQTTPORTMAX")
  {
    return "6";
  }
  else if (var == "MQTTUSER")
  {
    return String(DeviceConfig.MqttUser);
  }
  else if (var == "MPWD")
  {
    return String(DeviceConfig.MqttPwd);
  }
 
  else if (var == "MQTTPUB")
  {
    return String(DeviceConfig.MqttPub);
  }
  else if (var == "MQTTSUB")
  {
    return String(DeviceConfig.MqttSub);
  }
  else if (var == "MQTTPUBSEC")
  {
    return String(DeviceConfig.MqttPubSec);
  }
  return String();
}

// Set IP addresses
IPAddress apLocalIp(192, 168, 1, 1);
IPAddress apLocalIpGateway(192, 168, 1, 1);
IPAddress apLocalIpSubnet(255, 255, 255, 0);

uint64_t lastMsWiFiSta, lastMsHb;


DHT_Unified *dht = nullptr;
Simple_HCSR04 *hcsr = nullptr;
WiFiClient espClient;
PubSubClient client(espClient);


void setup()
{

  //Serial port for debugging purposes
  Serial.begin(115200);

  deviceInit();

  // Initialize SPIFFS
  if (!SPIFFS.begin())
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  loadConfig();
  WiFi.setHostname(DeviceConfig.DeviceName);
  WiFi.begin();

  pinMode(LED_HB, OUTPUT);

  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(DeviceConfig.ApSSID, DeviceConfig.ApPwd);

  // Route for web pages

  server.on(WEB_PAGE_ROOT, HTTP_GET, [](AsyncWebServerRequest *request) {
    handleRedirect(request);
  });
  server.on(WEB_PAGE_INDEX, HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!handleLoginAuth(request))
      return;
    request->send(SPIFFS, WEB_FILE_INDEX, CONTENT_TYPE_HTML, false, htmlProcess);
  });
  server.on(WEB_FAVICON, HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, WEB_FILE_FAVICON, CONTENT_TYPE_PNG, false);
  });
  server.on(WEB_PAGE_LOGIN, HTTP_ANY, [](AsyncWebServerRequest *request) {
    handleLoginPage(request);
  });
  server.on(WEB_PAGE_SETTINGS, HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!handleLoginAuth(request))
      return;
    request->send(SPIFFS, WEB_FILE_SETTINGS, CONTENT_TYPE_HTML, false, htmlProcess);
  });
  server.on(WEB_PAGE_LOGOUT, HTTP_GET, [](AsyncWebServerRequest *request) {
    handleLogout(request);
  });
  server.on(WEB_PAGE_REBOOT, HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!handleLoginAuth(request))
      return;
    request->send(SPIFFS, WEB_FILE_REBOOT, CONTENT_TYPE_HTML, false, htmlProcess);
  });
  server.on(WEB_PAGE_REBOOT_NOW, HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!handleLoginAuth(request))
      return;
    ESP.restart();
  });

  // Send a GET request to <ESP_IP>/updt
  server.on(WEB_PAGE_UPDATE, HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!handleLoginAuth(request))
      return;
    String resp = composeSensorJson();
    Serial.print("RESP: ");
    Serial.println(resp);
    request->send(200, CONTENT_TYPE_JSON, resp);
  });

  server.on(WEB_PAGE_SETTINGS_SAVE, HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!handleLoginAuth(request))
      return;
    for (int i = 0; i < request->params(); i++)
    {
      Serial.print("Prm:");
      Serial.println(request->getParam(i)->value());
      if (request->getParam(i)->name().equals(PARAM_INPUT_DEVICE_NAME))
      {
        String deviceName = request->getParam(i)->value();
        if (deviceName.length() > 0 && deviceName.length() <= MAX_CONF_STRING)
        {
          strcpy(DeviceConfig.DeviceName, deviceName.c_str());
        }
      }
      if (request->getParam(i)->name().equals(PARAM_INPUT_TETHERING_PWD))
      {
        String tetPwd = request->getParam(i)->value();
        if (tetPwd.length() > 0 && tetPwd.length() <= MAX_CONF_STRING)
        {
          strcpy(DeviceConfig.TetherPwd, tetPwd.c_str());
        }
      }
      if (request->getParam(i)->name().equals(PARAM_INPUT_AP_SSID))
      {
        String apSsid = request->getParam(i)->value();
        if (apSsid.length() > 0 && apSsid.length() <= MAX_CONF_SSID_NAME)
        {
          strcpy(DeviceConfig.ApSSID, apSsid.c_str());
        }
      }
      if (request->getParam(i)->name().equals(PARAM_INPUT_AP_PWD))
      {
        String apPwd = request->getParam(i)->value();
        if (apPwd.length() > 0 && apPwd.length() <= MAX_CONF_STRING)
        {
          strcpy(DeviceConfig.ApPwd, apPwd.c_str());
        }
      }
      if (request->getParam(i)->name().equals(PARAM_INPUT_PORTAL_USERNAME))
      {
        String pUser = request->getParam(i)->value();
        if (pUser.length() > 0 && pUser.length() <= MAX_CONF_STRING)
        {
          strcpy(DeviceConfig.PortalUser, pUser.c_str());
        }
      }
      if (request->getParam(i)->name().equals(PARAM_INPUT_PORTAL_PWD))
      {
        String pPwd = request->getParam(i)->value();
        if (pPwd.length() > 0 && pPwd.length() <= MAX_CONF_STRING)
        {
          strcpy(DeviceConfig.PortalPwd, pPwd.c_str());
        }
      }
      if (request->getParam(i)->name().equals(PARAM_INPUT_SENSOR_SEL))
      {
        String val = request->getParam(i)->value();

        if (val.length() > 0 && val.length() <= MAX_CONF_STRING)
        {
          DeviceConfig.Device = static_cast<DeviceSelect>(val.toInt());
        }
      }
      //mqtt
      if (request->getParam(i)->name().equals(PARAM_INPUT_MQTT_SERVER))
      {
        String val = request->getParam(i)->value();
        if (val.length() > 0 && val.length() <= MAX_CONF_STRING)
        {
          strcpy(DeviceConfig.MqttServer, val.c_str());
        }
      }
      if (request->getParam(i)->name().equals(PARAM_INPUT_MQTT_PORT))
      {
        String val = request->getParam(i)->value();
        if (val.length() > 0 && val.length() <= MAX_CONF_STRING)
        {
          DeviceConfig.MqttPort = val.toInt();
        }
      }
      if (request->getParam(i)->name().equals(PARAM_INPUT_MQTT_USER))
      {
        String val = request->getParam(i)->value();
        if (val.length() > 0 && val.length() <= MAX_CONF_STRING)
        {
          strcpy(DeviceConfig.MqttUser, val.c_str());
        }
      }
      if (request->getParam(i)->name().equals(PARAM_INPUT_MQTT_PWD))
      {
        String val = request->getParam(i)->value();
        if (val.length() > 0 && val.length() <= MAX_CONF_STRING)
        {
          strcpy(DeviceConfig.MqttPwd, val.c_str());
        }
      }
      if (request->getParam(i)->name().equals(PARAM_INPUT_MQTT_PUB_TOPIC))
      {
        String val = request->getParam(i)->value();
        if (val.length() > 0 && val.length() <= MAX_CONF_STRING)
        {
          strcpy(DeviceConfig.MqttPub, val.c_str());
        }
      }
      if (request->getParam(i)->name().equals(PARAM_INPUT_MQTT_SUB_TOPIC))
      {
        String val = request->getParam(i)->value();
        if (val.length() > 0 && val.length() <= MAX_CONF_STRING)
        {
          strcpy(DeviceConfig.MqttSub, val.c_str());
        }
      }
      if (request->getParam(i)->name().equals(PARAM_INPUT_MQTT_PUB_SEC))
      {
        String val = request->getParam(i)->value();
        if (val.length() > 0 && val.length() <= MAX_CONF_STRING)
        {
          DeviceConfig.MqttPubSec = val.toInt();
        }
      }
    }
    request->redirect(WEB_PAGE_ROOT);
    saveConfig();
    deviceInit();
    mqttInit();
    if (client.connected())
    {
      client.disconnect();
    }
  });

  server.serveStatic(WEB_PAGE_ASSETS, SPIFFS, WEB_PAGE_ASSETS);

  // Start server
  server.begin();

  lastMsWiFiSta = millis();

  mqttInit();
  mqttReconnect();
  Serial.println("Started...");
  
}

void deviceInit()
{
  Serial.println("Device init...");
  if (DeviceConfig.Device == DeviceSelect::DHT_11)
  {
    dht = new DHT_Unified(AIO_SENSOR_PIN_3, DHT11);
    Serial.print("Using DHT...");
    dht->begin();
    sensor_t sensor;
    dht->temperature().getSensor(&sensor);
    dht->humidity().getSensor(&sensor);
  }
  else if (DeviceConfig.Device == PIR || DeviceConfig.Device == Fire ||
           DeviceConfig.Device == Sound)
  {
    Serial.print("Using digital sensor...");
    pinMode(AIO_SENSOR_PIN_3, INPUT);
  }
  else if (DeviceConfig.Device == Relay)
  {
    Serial.print("Using relay...");
    pinMode(AIO_SENSOR_PIN_3, OUTPUT);
  }
  else if (DeviceConfig.Device == LDR)
  {
    Serial.print("Using LDR");
    pinMode(AIO_SENSOR_PIN_3, INPUT);
  }
  else if (DeviceConfig.Device == SoilMoisture)
  {
    Serial.print("Using Soil Moisture sensor...");
    pinMode(AIO_SENSOR_PIN_3, OUTPUT); 
  }
  else if (DeviceConfig.Device == Distance)
  {
    Serial.print("Using distance ensor...");
    hcsr = new Simple_HCSR04(DISTANCE_ECHO_PIN, DISTANCE_TRIG_PIN);
  }
}

void mqttInit()
{
  client.setServer(DeviceConfig.MqttServer, DeviceConfig.MqttPort);
  client.setCallback(mqttCallback);//fungsi yang akan diesekusi ketika mqtt mendapat data masuk
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  if (strcmp(topic, DeviceConfig.MqttSub) == 0)
  {
    Serial.print("Recvd relay command parse code: ");
    StaticJsonDocument<100> doc;
    DeserializationError error = deserializeJson(doc, (char *)payload);
    Serial.println(error.code());
    if (error == DeserializationError::Ok)
    {
      if (doc.containsKey("state") && doc["state"].is<int>())
      {
        DeviceConfig.RelayOn = (doc["state"].as<int>() == 1);
        Serial.print("Changing state: ");
        Serial.print(DeviceConfig.RelayOn );
        Serial.println();
      }
    }
  }
  
}

uint32_t lastReconnectMs = 0;
void mqttReconnect()
{
  if (!client.connected() && WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED &&
       millis() - lastReconnectMs > 5000)
  {
    Serial.print("Attempting MQTT connection...");
    String clientId = String(DeviceConfig.DeviceName) + "-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), DeviceConfig.MqttUser, DeviceConfig.MqttPwd))
    {
      Serial.println("connected");
      if (DeviceConfig.Device == Relay)
      {          
        client.setCallback(mqttCallback);
        Serial.println("Subscribing " + String(DeviceConfig.MqttSub) + (client.subscribe(DeviceConfig.MqttSub) ? " OK" : " FAIL"));
      }
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
    }
    lastReconnectMs = millis();
  }
}

#define FIRE_THRESH 1000

String composeSensorJson()
{
  DynamicJsonDocument doc(2048);

  JsonObject devData = doc.createNestedObject("data");

  if (DeviceConfig.Device == DeviceSelect::DHT_11)
  {
#ifdef DUMMY_SENSOR
    devData["humidity"] = "40 %";
    devData["temperature"] = "33.4 celcius";

#else
    if (dht == nullptr)
      deviceInit();
    sensors_event_t event;
    dht->temperature().getEvent(&event);
    if (isnan(event.temperature))
    {
      Serial.println(F("Error reading temperature!"));
      devData["temperature"] = "error";
    }
    else
    {
      Serial.print(F("Temperature: "));
      Serial.print(event.temperature);
      Serial.println(F("°C"));
      devData["temperature"] = String(event.temperature);// + " celcius";
    }
    // Get humidity event and print its value.
    dht->humidity().getEvent(&event);
    if (isnan(event.relative_humidity))
    {
      Serial.println(F("Error reading humidity!"));
      devData["humidity"] = "error";
    }
    else
    {
      Serial.print(F("Humidity: "));
      Serial.print(event.relative_humidity);
      Serial.println(F("%"));
      devData["humidity"] = String(event.relative_humidity) ;
    }

#endif
  }
  else if (DeviceConfig.Device == DeviceSelect::PIR)
  {
    devData["detection"] = (digitalRead(AIO_SENSOR_PIN_3) == PIR_ACTIVE) ? "Motion Detected" : "No Motion Detected";
  }
  else if (DeviceConfig.Device == DeviceSelect::Sound)
  {
    devData["analog_reading"] = (digitalRead(AIO_SENSOR_PIN_3) == PIR_ACTIVE ? "Sound detected" : "No Sound");
  }
  else if (DeviceConfig.Device == DeviceSelect::Fire)
  {
    devData["detection"] = (digitalRead(AIO_SENSOR_PIN_3) == PIR_ACTIVE ? "No Fire" : "Fire Detected");
  }
  else if (DeviceConfig.Device == DeviceSelect::LDR)
  {
    int ldrRawData;
    /*float resistorVoltage, ldrVoltage;
    float ldrResistance;
    float ldrLux;*/

    ldrRawData = analogRead(AIO_SENSOR_PIN_3);   //Baca sensor

    /*------------Konversi  dari data analog ke nilai lux------------//
    resistorVoltage = (float)ldrRawData / MAX_ADC_READING * ADC_REF_VOLTAGE;
    ldrVoltage = ADC_REF_VOLTAGE - resistorVoltage;
    ldrResistance = ldrVoltage / resistorVoltage * REF_RESISTANCE;
    ldrLux = LUX_CALC_SCALAR * pow(ldrResistance, LUX_CALC_EXPONENT);

    devData["intensity"] = String(ldrLux);*/
    devData["ldrRaw"] = String(ldrRawData);
  }
  else if(DeviceConfig.Device == DeviceSelect::SoilMoisture)
  {
    int sensor_analog;
    int moisture_percentage;
    sensor_analog = analogRead(AIO_SENSOR_PIN_3);
    moisture_percentage = (100 - (sensor_analog/4095.00) * 100);
    devData["moisture"] = String(moisture_percentage);
  }
  else if (DeviceConfig.Device == DeviceSelect::Relay)
  {
    devData["state"] = (DeviceConfig.RelayOn ? "ON" : "OFF");
  }
  else if (DeviceConfig.Device == DeviceSelect::Distance)
  {
#ifdef DUMMY_SENSOR
    devData["distance"] = "32.4 cm";
#else
    if (hcsr == nullptr)
      deviceInit();
    devData["distance"] = String(hcsr->measure()->cm()) + " cm";
#endif
  }
  doc["device_type"] = DEVICE_NAME[DeviceConfig.Device - 1];
  doc["local_ip"] = WiFi.localIP().toString();

  String ret;
  serializeJsonPretty(doc, ret);
  return ret;
}

bool toggleHb = true;
bool staConnected = false;
uint64_t lastSendMs = 0;
void loop()
{
  if (WiFi.getMode() == WIFI_STA && WiFi.status() != WL_CONNECTED && millis() - lastMsWiFiSta > CONNECT_STA_TIMEOUT)
  {
    Serial.print("Dropping AP mode ");
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apLocalIp, apLocalIpGateway, apLocalIpSubnet);
    WiFi.softAP(DeviceConfig.DeviceName, DeviceConfig.TetherPwd);
    WiFi.setHostname(DeviceConfig.DeviceName);
    staConnected = false;
    Serial.println("OK");
  }
  else if (WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED)
  {
    lastMsWiFiSta = millis();
    if (!staConnected)
    {
      staConnected = true;
      Serial.println(String("Connected to [") + WiFi.SSID() + "] IP: " + WiFi.localIP().toString());
      WiFi.setHostname(DeviceConfig.DeviceName);
    }
  }

  if (millis() - lastMsHb > HB_PERIOD_MS)
  {
    lastMsHb = millis();
    digitalWrite(LED_HB, !digitalRead(LED_HB));
  }

  mqttReconnect();
  if (client.connected() && millis() - lastSendMs >= (DeviceConfig.MqttPubSec * 1000))
  {
    Serial.print("Publishing:");
    String json = composeSensorJson();
    Serial.println(json);
    Serial.println(client.publish(DeviceConfig.MqttPub, json.c_str()) ? "OK" : "FAIL");
    lastSendMs = millis();
  }
  if (DeviceConfig.Device == Relay) {
    digitalWrite(AIO_SENSOR_PIN_3, DeviceConfig.RelayOn ? RELAY_ACTIVE : RELAY_INACTIVE);
  }
  client.loop();
}
