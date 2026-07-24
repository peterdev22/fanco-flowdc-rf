#ifndef CONFIG_H
#define CONFIG_H
#include <stdint.h>

// fan config
const uint16_t FAN_ID = 0b0000000000000000; // exactly 16 bits

// network
const char* DEVICE_NAME = "Ceiling Fan";
const char* DEVICE_ID   = "ceiling_fan_bedroom";

const char* WIFI_SSID = "";
const char* WIFI_PASS = "";

const char* MQTT_URI  = "";
const char* MQTT_USER = "";
const char* MQTT_PASS = "";
const char* MQTT_CERT = "";

#endif