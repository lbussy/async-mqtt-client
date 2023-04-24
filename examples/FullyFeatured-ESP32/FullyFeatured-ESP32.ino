#include <LCBUrl.h>
#include <Arduino.h>
#include <list>
#include <ArduinoLog.h>
#include <WiFi.h>
#include <Ticker.h>

#ifdef LCBURL_MDNS
#include <ESPmDNS.h>
#endif

#include "AsyncMqttClient.h"

#if __has_include("./secrets.h")
#include "secrets.h" // Include for AP_NAME and PASSWD below
const char *ssid = AP_NAME;
const char *password = PASSWRD;
const char *hostname = HOSTNAME;
const char *mqtt_host = MQTT_HOST;
const int mqtt_port = MQTT_PORT;
const int baud = BAUD;
#else
const char *ssid = "my_ap";
const char *password = "passsword";
const char *hostname = "test-host";
const char *mqtt_host = "raspberrypi.local";
const int mqtt_port = 1883;
const int baud = 115200;
#endif

LCBUrl url;
bool oktomqt;

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

void connectToWifi()
{
    Log.notice("Connecting to Wi-Fi." CR);
    WiFi.setAutoReconnect(false);
    WiFi.setSleep(false);
    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin(ssid, password);
}

void connectToMqtt()
{
    Log.notice("Connecting to MQTT." CR);
    mqttClient.connect();
}

void WiFiEvent(WiFiEvent_t event)
{
    Log.notice("[WiFi-event] event: %d" CR, event);
    switch (event)
    {
    case SYSTEM_EVENT_STA_GOT_IP:
        Log.notice("WiFi connected. IP address: %s" CR, WiFi.localIP().toString().c_str());
        if (oktomqt)
            connectToMqtt();
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        Log.notice("WiFi lost connection." CR);
        // Ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
        mqttReconnectTimer.detach();
        connectToWifi();
        break;
    }
}

void onMqttConnect(bool sessionPresent)
{
    Log.notice("Connected to MQTT." CR);
    Log.notice("Session present: %T" CR, sessionPresent);
    uint16_t packetIdSub = mqttClient.subscribe("test/lol", 2);
    Log.notice("Subscribing at QoS 2, packetId: %d" CR, packetIdSub);
    mqttClient.publish("test/lol", 0, true, "test 1");
    Log.notice("Publishing at QoS 0" CR);
    uint16_t packetIdPub1 = mqttClient.publish("test/lol", 1, true, "test 2");
    Log.notice("Publishing at QoS 1, packetId: %d" CR, packetIdPub1);
    uint16_t packetIdPub2 = mqttClient.publish("test/lol", 2, true, "test 3");
    Log.notice("Publishing at QoS 2, packetId: %d" CR, packetIdPub2);
    mqttReconnectTimer.detach();
    mqttReconnectTimer.attach(2, connectToMqtt);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
    Log.notice("Disconnected from MQTT." CR);
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos)
{
    Log.notice("Subscribe acknowledged." CR);
    Log.notice("\tpacketId: %d" CR, packetId);
    Log.notice("\tqos: %d" CR, qos);
}

void onMqttUnsubscribe(uint16_t packetId)
{
    Log.notice("Unsubscribe acknowledged." CR);
    Log.notice("\tpacketId: %d" CR, packetId);
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
    Log.notice("Publish received." CR);
    Log.notice("\ttopic: %s", topic);
    Log.notice("\tqos: %d" CR, properties.qos);
    Log.notice("\tdup: %T" CR, properties.dup);
    Log.notice("\tretain: %T" CR, properties.retain);
    Log.notice("\tlen: %d" CR, len);
    Log.notice("\tindex: " CR, index);
    Log.notice("\ttotal: %d" CR, total);
}

void onMqttPublish(uint16_t packetId)
{
    Log.notice("Publish acknowledged.");
    Log.notice("\tpacketId: %d" CR, packetId);
}

void setup()
{
    oktomqt = false;
    Serial.begin(BAUD);
    Serial.flush();
    Serial.println();
    delay(1000);

    // Initialize with log level and log output.
    Log.begin(LOG_LEVEL_VERBOSE, &Serial);
    Log.verbose(F("Starting." CR));

    WiFi.onEvent(WiFiEvent);
    connectToWifi();
    while (!WiFi.isConnected()) {;}

    MDNS.begin(hostname);

    url.setUrl(mqtt_host);
    while (url.getIP(mqtt_host) == INADDR_NONE) {;}

    Log.verbose("Setting MQTT Host as %s, (%s)" CR, mqtt_host, url.getIP(mqtt_host).toString().c_str());
    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onSubscribe(onMqttSubscribe);
    mqttClient.onUnsubscribe(onMqttUnsubscribe);
    mqttClient.onMessage(onMqttMessage);
    mqttClient.onPublish(onMqttPublish);
    mqttClient.setServer(url.getIP(mqtt_host), mqtt_port);
    connectToMqtt();
    oktomqt = true;
}

void loop()
{
}
