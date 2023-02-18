#ifndef STORAGE_HPP
#define STORAGE_HPP

#include <Arduino.h>
#include <definitions.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#define CONFIG_FILE "/data/settings.json"

#define JSON_DEV_NAME_KEY "NAME"
#define JSON_AP_NAME_KEY "APNAME"
#define JSON_AP_PWD_KEY "APPWD"
#define JSON_TET_PWD_KEY "TETPWD"
#define JSON_PORTAL_USER_KEY "PORTALUSER"
#define JSON_PORTAL_PWD_KEY "PORTALPWD"

#define JSON_MQTT_SERVER_KEY "MQTT_SERVER"
#define JSON_MQTT_PORT_KEY "MQTT_PORT"
#define JSON_MQTT_USERNAME_KEY "MQTT_USERNAME"
#define JSON_MQTT_PWD_KEY "MQTT_PWD"
#define JSON_MQTT_PUB_TOPIC_KEY "MQTT_PUB_TOPIC"
#define JSON_MQTT_SUB_TOPIC_KEY "MQTT_SUB_TOPIC"
#define JSON_MQTT_PUB_SEC_KEY "MQTT_PUB_SEC"
#define JSON_MQTT_SENSOR_SEL_KEY "SENSOR"


void defaultConfig();
void loadConfig();
void saveConfig();

void saveConfig()
{
#if (ARDUINOJSON_VERSION_MAJOR >= 6)
    DynamicJsonDocument json(2048);
#else
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.createObject();
#endif
    json[JSON_DEV_NAME_KEY] = DeviceConfig.DeviceName;
    json[JSON_TET_PWD_KEY] = DeviceConfig.TetherPwd;
    json[JSON_AP_NAME_KEY] = DeviceConfig.ApSSID;
    json[JSON_AP_PWD_KEY] = DeviceConfig.ApPwd;
    json[JSON_PORTAL_USER_KEY] = DeviceConfig.PortalUser;
    json[JSON_PORTAL_PWD_KEY] = DeviceConfig.PortalPwd;

    json[JSON_MQTT_SERVER_KEY] = DeviceConfig.MqttServer;
    json[JSON_MQTT_PORT_KEY] = DeviceConfig.MqttPort;
    json[JSON_MQTT_USERNAME_KEY] = DeviceConfig.MqttUser;
    json[JSON_MQTT_PWD_KEY] = DeviceConfig.MqttPwd;
    json[JSON_MQTT_PUB_TOPIC_KEY] = DeviceConfig.MqttPub;
    json[JSON_MQTT_SUB_TOPIC_KEY] = DeviceConfig.MqttSub;
    json[JSON_MQTT_PUB_SEC_KEY] = DeviceConfig.MqttPubSec;
    json[JSON_MQTT_SENSOR_SEL_KEY] = (int)DeviceConfig.Device;
    json[JSON_MQTT_PUB_SEC_KEY] = DeviceConfig.MqttPubSec;
    // Open file for writing
    fs::File f = SPIFFS.open(CONFIG_FILE, "w");

    if (!f)
    {
        return;
    }

#if (ARDUINOJSON_VERSION_MAJOR >= 6)
    //serializeJsonPretty(json, Serial);
    // Write data to file and close it
    serializeJson(json, f);
#else
    // json.prettyPrintTo(Serial);
    // Write data to file and close it
    json.printTo(f);
#endif

    f.close();
    json.clear();

    Serial.println(FPSTR("\n[CONF] Extra config file was successfully saved"));
}

void loadConfig()
{
    if (!SPIFFS.exists(CONFIG_FILE))
    {
        defaultConfig();
        return;
    }
    // this opens the config file in read-mode
    fs::File f = SPIFFS.open(CONFIG_FILE, "r");

    if (!f)
    {
        Serial.println(FPSTR("[CONF] General config NOT FOUND"));
        defaultConfig();
        return;
    }
    else
    {
        // we could open the file
        size_t size = f.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size + 1]);

        // Read and store file contents in buf
        f.readBytes(buf.get(), size);
        // Closing file
        f.close();
        // Using dynamic JSON buffer which is not the recommended memory model, but anyway
        // See https://github.com/bblanchon/ArduinoJson/wiki/Memory%20model

#if (ARDUINOJSON_VERSION_MAJOR >= 6)
        DynamicJsonDocument json(2048);
        auto deserializeError = deserializeJson(json, buf.get());
        if (deserializeError)
        {
            Serial.println(FPSTR("[CONF] General config ERROR"));
            defaultConfig();
            return;
        }
        serializeJsonPretty(json, Serial);
#else
        DynamicJsonBuffer jsonBuffer;
        // Parse JSON string
        JsonObject &json = jsonBuffer.parseObject(buf.get());
        // Test if parsing succeeds.
        if (!json.success())
        {
            Serial.println("JSON parseObject() failed");
            return false;
        }
        // json.printTo(Serial);
#endif

        // Parse all config file parameters, override
        // local config variables with parsed values

        bool configsValid = true;
        // Validity check general
        configsValid &= (json.containsKey(JSON_DEV_NAME_KEY) &&
                         json[JSON_DEV_NAME_KEY].size() <= MAX_CONF_STRING &&
                         json.containsKey(JSON_AP_NAME_KEY) &&
                         json[JSON_AP_NAME_KEY].size() <= MAX_CONF_SSID_NAME &&
                         json.containsKey(JSON_AP_PWD_KEY) &&
                         json[JSON_AP_PWD_KEY].size() <= MAX_CONF_STRING &&
                         json.containsKey(JSON_TET_PWD_KEY) &&
                         json[JSON_TET_PWD_KEY].size() <= MAX_CONF_STRING &&
                         json.containsKey(JSON_PORTAL_USER_KEY) &&
                         json[JSON_PORTAL_USER_KEY].size() <= MAX_CONF_STRING &&
                         json.containsKey(JSON_PORTAL_PWD_KEY) &&
                         json[JSON_PORTAL_PWD_KEY].size() <= MAX_CONF_STRING &&
                         
                         json.containsKey(JSON_MQTT_SERVER_KEY) &&
                         json[JSON_MQTT_SERVER_KEY].size() <= MAX_CONF_STRING &&
                         json.containsKey(JSON_MQTT_PORT_KEY) &&
                         json.containsKey(JSON_MQTT_USERNAME_KEY) &&
                         json[JSON_MQTT_USERNAME_KEY].size() <= MAX_CONF_STRING &&
                         json.containsKey(JSON_MQTT_PWD_KEY) &&
                         json[JSON_MQTT_PWD_KEY].size() <= MAX_CONF_STRING &&
                         json.containsKey(JSON_MQTT_PUB_TOPIC_KEY) &&
                         json[JSON_MQTT_PUB_TOPIC_KEY].size() <= MAX_CONF_STRING &&
                         json.containsKey(JSON_MQTT_SUB_TOPIC_KEY) &&
                         json[JSON_MQTT_SUB_TOPIC_KEY].size() <= MAX_CONF_STRING &&
                         json.containsKey(JSON_MQTT_SENSOR_SEL_KEY) &&
                         json.containsKey(JSON_MQTT_PUB_SEC_KEY) &&
                         json[JSON_MQTT_PUB_SEC_KEY].is<int>())
                         ;
        if (!configsValid)
        {
            Serial.println(FPSTR("[CONF] General config DEFAULT"));
            defaultConfig();
            return;
        }

        Serial.println(FPSTR("[CONF] General config OK"));

        strcpy(DeviceConfig.DeviceName, json[JSON_DEV_NAME_KEY]);
        strcpy(DeviceConfig.ApSSID, json[JSON_AP_NAME_KEY]);
        strcpy(DeviceConfig.ApPwd, json[JSON_AP_PWD_KEY]);
        strcpy(DeviceConfig.TetherPwd, json[JSON_TET_PWD_KEY]);
        strcpy(DeviceConfig.PortalUser, json[JSON_PORTAL_USER_KEY]);
        strcpy(DeviceConfig.PortalPwd, json[JSON_PORTAL_PWD_KEY]);

        strcpy(DeviceConfig.MqttServer, json[JSON_MQTT_SERVER_KEY]);
        DeviceConfig.MqttPort = json[JSON_MQTT_PORT_KEY];
        strcpy(DeviceConfig.MqttUser, json[JSON_MQTT_USERNAME_KEY]);
        strcpy(DeviceConfig.MqttPwd, json[JSON_MQTT_PWD_KEY]);
        strcpy(DeviceConfig.MqttPub, json[JSON_MQTT_PUB_TOPIC_KEY]);
        strcpy(DeviceConfig.MqttSub, json[JSON_MQTT_SUB_TOPIC_KEY]);
        DeviceConfig.MqttPubSec = json[JSON_MQTT_PUB_SEC_KEY];
        DeviceConfig.Device = (DeviceSelect)json[JSON_MQTT_SENSOR_SEL_KEY];
    }
}

void defaultConfig()
{
    Config newConfig;
    memcpy(&DeviceConfig, &newConfig, sizeof(Config));
    saveConfig();
}

#endif