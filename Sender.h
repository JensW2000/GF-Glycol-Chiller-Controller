/*
---------------------------------------------------------------------------------------------------------
Special thanks to Samuel Lang.
This library is borrowed from his iSpindel project (https://github.com/universam1/iSpindel).
---------------------------------------------------------------------------------------------------------
*/


#ifndef _SENDER_H_
#define _SENDER_H_

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

class SenderClass
{
public:
  SenderClass();
  bool hasData();
  void sendTCP(String server, uint16_t port = 80);
  void sendGenericPost(String server, String url, uint16_t port = 80);
  void sendInfluxDB(String server, uint16_t port, String db, String name);
  void sendPrometheus(String server, uint16_t port, String job, String instance);
  void sendUbidots(String token, String name);
  void sendFHEM(String server, uint16_t port, String name);
  void sendTCONTROL(String server, uint16_t port);
  void add(String id, float value);
  void add(String id, String value);
  void add(String id, int32_t value);
  void add(String id, uint32_t value);
  void add(String id, ulong value);
  // ~SenderClass();

private:
  WiFiClient _client;
  // StaticJsonBuffer<200> _jsonBuffer;
  DynamicJsonBuffer _jsonBuffer;
  // JsonObject data;
  JsonVariant _jsonVariant;
};

#endif
