#ifndef DEFINITION_H
#define DEFINITION_H

#define FW_VER "1.0.0";
#define PRODUCT_NAME "Sensor Device MQTT";

// Set to true to define Relay as Normally Open (NO)
#define MAX_CONF_STRING 31
#define MAX_CONF_STRING_LONG 100
#define MAX_CONF_SSID_NAME  31

#define CONNECT_STA_TIMEOUT 30000

#define RELAY_ON_TIMEOUT_PERIOD_MS  1000 

#define LED_HB  2
#define HB_PERIOD_MS 500

#define AIO_SENSOR_PIN_1 2
#define AIO_SENSOR_PIN_2 5
#define AIO_SENSOR_PIN_3 32
#define DISTANCE_TRIG_PIN AIO_SENSOR_PIN_1
#define DISTANCE_ECHO_PIN AIO_SENSOR_PIN_2
#define PIR_ACTIVE HIGH
#define RELAY_ACTIVE LOW
#define RELAY_INACTIVE HIGH
#define RELAY_MQTT_CMD_TOPIC "/command"
#define MQTT_PUB_SEC_DEFAULT 5

/*ldr
#define MAX_ADC_READING 4095
#define ADC_REF_VOLTAGE 5.0
#define REF_RESISTANCE 330
#define LUX_CALC_SCALAR 125235178.3654270
#define LUX_CALC_EXPONENT -1.604568157*/


// #define DUMMY_SENSOR

typedef enum devsel {
    DHT_11 = 1,
    SoilMoisture, PIR, LDR, Relay, Distance, Sound, Fire
} DeviceSelect;

String DEVICE_NAME[8] = {"DHT11", "SoilMoisture", "PIR", "LDR", "Relay", "Distance", "Sound", "Fire"};

// Replace with your network credentials
const char *ssid = "asd";
const char *password = "12345678";

const char *PARAM_INPUT_RELAY = "rly";
const char *PARAM_INPUT_STATE = "st";

const char *PARAM_INPUT_DEVICE_NAME = "dnm";
const char *PARAM_INPUT_TETHERING_PWD= "tetpwd";
const char *PARAM_INPUT_AP_SSID = "apssid";
const char *PARAM_INPUT_AP_PWD = "appwd";
const char *PARAM_INPUT_PORTAL_USERNAME = "puser";
const char *PARAM_INPUT_PORTAL_PWD = "ppwd";
const char *PARAM_INPUT_TRIGGER_SEC = "trigsec";
const char *PARAM_INPUT_SENSOR_SEL = "sensor";
const char *PARAM_INPUT_MQTT_SERVER = "msrvr";
const char *PARAM_INPUT_MQTT_PORT = "mport";
const char *PARAM_INPUT_MQTT_USER = "muser";
const char *PARAM_INPUT_MQTT_PWD = "mpwd";
const char *PARAM_INPUT_MQTT_PUB_TOPIC = "mpub";
const char *PARAM_INPUT_MQTT_SUB_TOPIC = "msub";
const char *PARAM_INPUT_MQTT_PUB_SEC = "mpubsec";

const char *WEB_PAGE_ROOT = "/";
const char *WEB_PAGE_ASSETS = "/assets";
const char *WEB_PAGE_INDEX = "/main";
const char *WEB_PAGE_LOGIN = "/login";
const char *WEB_PAGE_SETTINGS = "/settings";
const char *WEB_PAGE_SETTINGS_SAVE = "/save";
const char *WEB_PAGE_LOGOUT = "/logout";
const char *WEB_PAGE_UPDATE = "/updt";
const char *WEB_PAGE_REBOOT = "/reboot";
const char *WEB_PAGE_REBOOT_NOW = "/rebootNow";
const char *WEB_FAVICON = "/favicon.ico";

const char *WEB_FILE_INDEX = "/index.html";
const char *WEB_FILE_LOGIN = "/login.html";
const char *WEB_FILE_SETTINGS = "/settings.html";
const char *WEB_FILE_REBOOT = "/reboot.html";
const char *WEB_FILE_FAVICON = "/assets/logo.png";
const char *WEB_FILE_SETTINGS_CSS = "/settings.css";

const char *CONTENT_TYPE_HTML = "text/html";
const char *CONTENT_TYPE_JSON = "application/json";
const char *CONTENT_TYPE_CSS = "text/css";
const char *CONTENT_TYPE_PNG = "image/png";
const char *CONTENT_TYPE_JS = "text/javascript";


typedef struct conf {
    char DeviceName[MAX_CONF_STRING + 1] = "Deployer 5";
    char ApSSID[MAX_CONF_SSID_NAME + 1] = "realme";
    char ApPwd[MAX_CONF_STRING + 1] = "biasaaja";
    char TetherPwd[MAX_CONF_STRING + 1] = "tugasakhir";
    char PortalUser[MAX_CONF_STRING + 1] = "admin";
    char PortalPwd[MAX_CONF_STRING + 1] = "admin";

    char MqttServer[MAX_CONF_STRING + 1] = "192.168.147.216";
    uint16_t MqttPort = 1883;
    char MqttUser[MAX_CONF_STRING + 1] = "";
    char MqttPwd[MAX_CONF_STRING + 1] = "";
    char MqttPub[MAX_CONF_STRING + 1] = "/feeds/sensor";
    char MqttSub[MAX_CONF_STRING + 1] = RELAY_MQTT_CMD_TOPIC;
    int MqttPubSec = MQTT_PUB_SEC_DEFAULT;
    DeviceSelect Device = DHT_11;
    bool RelayOn = false;
} Config;

static Config DeviceConfig;
#endif