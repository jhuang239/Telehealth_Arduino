#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

#define RXp2 16
#define TXp2 17

//const char* ssid = "EE3070_P1615_1";
//const char* password = "EE3070P1615";
const char* ssid = "Kim";
const char* password = "Kim.W?0591";

const char* root_ca = \
                      "-----BEGIN CERTIFICATE-----\n" \
                      "MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n" \
                      "ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n" \
                      "b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n" \
                      "MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n" \
                      "b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n" \
                      "ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n" \
                      "9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n" \
                      "IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n" \
                      "VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n" \
                      "93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n" \
                      "jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n" \
                      "AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n" \
                      "A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n" \
                      "U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n" \
                      "N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n" \
                      "o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n" \
                      "5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n" \
                      "rqXRfboQnoZsG4q5WTP468SQvvG5\n" \
                      "-----END CERTIFICATE-----\n";


//MQTT
const char* brokerUrl = "driver.cloudmqtt.com";
int Port = 18712;
const char* userMQTT = "cgxgibnr";
const char* passMQTT = "MzxIi8_n_rr_";
const char* subUser = "EE3070GP2/user";
const char* outTopic = "EE3070GP2/outTopic";
const String clienteId = "ESP32EE3070MQTT";
uint16_t keepAlive = 600;

WiFiClient espClient;
PubSubClient clientMqtt(espClient);

void reconnect() {
  while (!clientMqtt.connected()) {
    Serial.print("\nConnecting to ");
    Serial.println(brokerUrl);
    if (clientMqtt.connect(clienteId.c_str(), userMQTT, passMQTT)) {
      Serial.print("\nConnected to ");
      Serial.print(brokerUrl);
      Serial.println();
      clientMqtt.subscribe(subUser);
      //      clientMqtt.subscribe(subCommand);
    } else {
      Serial.println(clientMqtt.state());
      Serial.println("\nTrying connect again!");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String s_topic = String(topic);
  String message = "";
  if (s_topic == String(subUser)) {
    Serial.print("Received messages: ");
    Serial.println(topic);
    for (int i = 0; i < length; i++) {
      message += String((char)payload[i]);
    }
    Serial.println(message);
    String jsonData = packJson(message);
    Serial2.flush();
    Serial2.println(jsonData);
    String backMessage = Serial2.readString();
    while (!unpackBackMessage(backMessage)) {
      backMessage = Serial2.readString();
    }
    upload(backMessage);
    delay(3000);
    clientMqtt.publish(outTopic, "finished");
  }
}

boolean unpackBackMessage(String jsonMessage) {
  StaticJsonDocument<6144> doc;
  DeserializationError error = deserializeJson(doc, jsonMessage);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return false;
  }
  String command = doc["command"];
  if (command == "upload") {
    return true;
  } else {
    return false;
  }
}

void upload(String jsonData) {
  if ((WiFi.status() == WL_CONNECTED) && jsonData.length() > 0) {
    HTTPClient http;
    http.begin("https://ee3070t2.herokuapp.com/addHealthData", root_ca);
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST(jsonData);
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println(payload);
    }
    else {
      Serial.println("Error on HTTP request");
    }
    http.end();
  }
}

String packJson(String username) {
  String output;
  StaticJsonDocument<64> doc;
  doc["command"] = "start_meausre";
  doc["username"] = username;
  serializeJson(doc, output);

  return output;
}

void setup() {
  Serial.begin(9600);
  Serial2.begin(115200, SERIAL_8N1, RXp2, TXp2);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  clientMqtt.setServer(brokerUrl, Port);
  clientMqtt.setCallback(callback);
  clientMqtt.setKeepAlive(keepAlive);
}

void loop() {
  if (!clientMqtt.connected()) {
    reconnect();
  }
  clientMqtt.loop();
}
