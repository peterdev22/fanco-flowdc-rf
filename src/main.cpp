#include <Arduino.h>
#include <SmartRC_CC1101.h>
#include <SPI.h>
#include <config.h>
#include <mqtt_client.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#define CS D3
#define MOSI D10
#define SCK D8
#define MISO D9
#define GDO0 D2

SmartRC_CC1101 radio;
esp_mqtt_client_handle_t mqtt_client;

enum FanMode {
    FAN_TIMER_1H    = 0,
    FAN_TIMER_4H    = 1,
    FAN_SPEED_1     = 2,
    FAN_SPEED_2     = 3,
    FAN_SPEED_3     = 4,
    FAN_SPEED_4     = 5,
    FAN_SPEED_5     = 6,
    FAN_SPEED_6     = 7,
    FAN_DIR         = 9,
    LIGHT_POWER     = 10,
    LIGHT_BRIGHTEN  = 12,
    LIGHT_DIM       = 13,
    FAN_TIMER_8H    = 14,
    FAN_SPEED_MIXED = 15,
    FAN_POWER       = 17
};

struct Pulse
{
  bool level;
  uint16_t duration;
};

const Pulse ZERO[] =
{ 
  {HIGH, 387}, 
  {LOW, 801} 
};
const Pulse ONE[] = 
{ 
  {HIGH, 935}, 
  {LOW, 230} 
};
const Pulse END[] = 
{ 
  {HIGH, 347}, 
  {LOW, 4326} 
};

Pulse frame[66];

char cmd_topic[64];
char availability_topic[64];

// --------

void buildFrame(FanMode fan_mode, uint16_t fan_id)
{ 
  uint8_t pos = 0;

  // command (fan mode dec repeated twice)
  uint16_t command = (fan_mode << 8) | fan_mode;

  for (int i = 15; i >= 0; --i)
  {
    bool bit = (command >> i) & 1;

    if (bit == LOW) {
      frame[pos++] = ZERO[0];
      frame[pos++] = ZERO[1];
    } else {
      frame[pos++] = ONE[0];
      frame[pos++] = ONE[1];
    }
  }

  // fan id
  for (int j = 15; j >= 0; --j)
  {
    bool bit = (fan_id >> j) & 1;

    if (bit == LOW) {
      frame[pos++] = ZERO[0];
      frame[pos++] = ZERO[1];
    } else {
      frame[pos++] = ONE[0];
      frame[pos++] = ONE[1];
    }
  }

  // gap between frames
  frame[pos++] = END[0];
  frame[pos++] = END[1];
}

void transmitPulse(Pulse p)
{
  digitalWrite(GDO0, p.level);
  delayMicroseconds(p.duration);
}

void sendFrame(int8_t frame_count = 1)
{
  radio.SetTx();
  delayMicroseconds(100);

  for (int i = 0; i < frame_count; i++)
  {
    for (unsigned int j = 0; j < 66; j++)
    {
      transmitPulse(frame[j]);
    }

    digitalWrite(GDO0, 0);
  }
}

// --------------

static void publishDevice()
{

  char config_topic[64];
  char payload[1024];

  snprintf(config_topic, sizeof(config_topic), "homeassistant/device/%s/config", DEVICE_ID);
  snprintf(cmd_topic, sizeof(cmd_topic), "devices/%s/set", DEVICE_ID);
  snprintf(availability_topic, sizeof(availability_topic), "devices/%s/availability", DEVICE_ID);

  JsonDocument doc;

  JsonObject device = doc["dev"].to<JsonObject>();
  device["ids"] = DEVICE_ID;
  device["name"] = DEVICE_NAME;
  device["mf"] = "Fanco";
  device["mdl"] = "Flow DC";

  JsonObject origin = doc["o"].to<JsonObject>();
  origin["name"] = "fanco-flowdc-rf";
  origin["sw"] = "1.0";

  JsonObject fan = doc["cmps"]["fan"].to<JsonObject>();
  fan["p"] = "fan";
  fan["name"] = DEVICE_NAME;
  fan["uniq_id"] = DEVICE_ID;
  fan["cmd_t"] = cmd_topic;
  fan["pl_on"] = "ON";
  fan["pl_off"] = "OFF";
  fan["spd_cmd_t"] = cmd_topic;
  fan["spd_rng_max"] = 6;
  fan["avty_t"] = availability_topic;
  fan["pl_al_avail"] = "ONLINE";
  fan["pl_not_avail"] = "OFFLINE";
  fan["opt"] = true;
  fan["qos"] = 1;

  size_t payload_size = serializeJson(doc, payload, sizeof(payload));

  esp_mqtt_client_publish(mqtt_client, config_topic, payload, payload_size, 1, true);
  Serial.println("[!] Published initial device details.");
}

static void mqttEventHandler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data)
{
  esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);

  switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
      Serial.println("[!] MQTT connected.");

      publishDevice();

      esp_mqtt_client_subscribe(mqtt_client, cmd_topic, 1);
      Serial.println("[!] Listening to commands.");
      esp_mqtt_client_publish(mqtt_client, availability_topic, "ONLINE", 0, 1, true);
      Serial.println("[!] Published online status.");

      break;
    case MQTT_EVENT_DISCONNECTED:
      Serial.println("[!] MQTT disconnected.");
      break;
    case MQTT_EVENT_DATA:
      Serial.print("[!] Command received: ");
      Serial.write(event->data, event->data_len);
      Serial.println();

      if (strncmp(event->data, "ON", event->data_len) == 0) {
        buildFrame(FAN_SPEED_1, FAN_ID);
        sendFrame(5);
      } else if (strncmp(event->data, "OFF", event->data_len) == 0) {
        buildFrame(FAN_POWER, FAN_ID);
        sendFrame(5);
      } else if (strncmp(event->data, "1", event->data_len) == 0) {
        buildFrame(FAN_SPEED_1, FAN_ID);
        sendFrame(5);
      } else if (strncmp(event->data, "2", event->data_len) == 0) {
        buildFrame(FAN_SPEED_2, FAN_ID);
        sendFrame(5);
      } else if (strncmp(event->data, "3", event->data_len) == 0) {
        buildFrame(FAN_SPEED_3, FAN_ID);
        sendFrame(5);
      } else if (strncmp(event->data, "4", event->data_len) == 0) {
        buildFrame(FAN_SPEED_4, FAN_ID);
        sendFrame(5);
      } else if (strncmp(event->data, "5", event->data_len) == 0) {
        buildFrame(FAN_SPEED_5, FAN_ID);
        sendFrame(5);
      } else if (strncmp(event->data, "6", event->data_len) == 0) {
        buildFrame(FAN_SPEED_6, FAN_ID);
        sendFrame(5);
      }

      break;
    case MQTT_EVENT_ERROR:
      Serial.println("[!] MQTT error.");
      break;
    default:
      break;
  }
}

static void initMQTT()
{
  esp_mqtt_client_config_t mqtt_cfg = {};
  mqtt_cfg.broker.address.uri = MQTT_URI;
  mqtt_cfg.broker.verification.certificate = MQTT_CERT;
  mqtt_cfg.credentials.client_id = DEVICE_ID;
  mqtt_cfg.credentials.username = MQTT_USER;
  mqtt_cfg.credentials.authentication.password = MQTT_PASS;
  mqtt_cfg.session.keepalive = 30;
  mqtt_cfg.session.last_will.topic = availability_topic;
  mqtt_cfg.session.last_will.msg = "OFFLINE";
  mqtt_cfg.session.last_will.qos = 1;
  mqtt_cfg.session.last_will.retain = true;

  mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqttEventHandler, nullptr);
  esp_mqtt_client_start(mqtt_client);
}

static void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("[!] Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.println();
  Serial.print("[!] Connected successfully. Local IP Address: ");
  Serial.println(WiFi.localIP());
}

// -------------------

void setup()
{ 
  delay(1000);
  Serial.begin(115200);

  pinMode(GDO0, OUTPUT);
  digitalWrite(GDO0, 0);


  SPI.begin(SCK, MISO, MOSI, CS);

  radio.setSpiPin(SCK, MISO, MOSI, CS);
  radio.setGDO0(GDO0);

  radio.Init();

  radio.setCCMode(0);
  radio.setModulation(2);
  radio.setMHZ(433.92);

  radio.SpiWriteReg(CC1101_PKTCTRL0, 0x32);
  radio.SpiWriteReg(CC1101_IOCFG0, 0x0D);
  radio.SpiWriteReg(CC1101_PKTCTRL1, 0x00);

  radio.SpiWriteReg(CC1101_MDMCFG4, 0xF8);
  radio.SpiWriteReg(CC1101_MDMCFG3, 0x83);

  radio.SetTx();

  pinMode(GDO0, OUTPUT);
  
  delay(1000);

  initWiFi();
  initMQTT();

  delay(5000);
}

void loop()
{

}