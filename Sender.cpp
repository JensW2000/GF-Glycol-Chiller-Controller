/*
---------------------------------------------------------------------------------------------------------
Special thanks to Samuel Lang.
This library is borrowed from his iSpindel project (https://github.com/universam1/iSpindel).
---------------------------------------------------------------------------------------------------------
*/


#include "Sender.h"
#include <ArduinoJson.h>

#define UBISERVER "things.ubidots.com"
#define CONNTIMEOUT 2000


SenderClass::SenderClass()
{
    _jsonVariant = _jsonBuffer.createObject();
}
bool SenderClass::hasData()
{
	return _jsonVariant.success();
}
void SenderClass::add(String id, float value)
{
    _jsonVariant[id] = value;
}
void SenderClass::add(String id, String value)
{
    _jsonVariant[id] = value;
}
void SenderClass::add(String id, uint32_t value)
{
    _jsonVariant[id] = value;
}
void SenderClass::add(String id, int32_t value)
{
    _jsonVariant[id] = value;
}
void SenderClass::add(String id, ulong value)
{
	_jsonVariant[id] = value;
}
void SenderClass::sendTCP(String server, uint16_t port)
{
    _jsonVariant.printTo(Serial);

    if (_client.connect(server.c_str(), port))
    {
        _jsonVariant.printTo(_client);
        _client.println();
    }
    else
    {
    }

    int timeout = 0;
    while (!_client.available() && timeout < CONNTIMEOUT)
    {
        timeout++;
        delay(1);
    }
    while (_client.available())
    {
        char c = _client.read();
        Serial.write(c);
    }
    // currentValue = 0;
    _client.stop();
    delay(100); // allow gracefull session close
}

void SenderClass::sendGenericPost(String server, String url, uint16_t port)
{
    _jsonVariant.printTo(Serial);

    HTTPClient http;

    // configure traged server and url
    http.begin(server, port, url);
    http.addHeader("User-Agent", "iSpindel");
    http.addHeader("Connection", "close");
    http.addHeader("Content-Type", "application/json");

    String json;
    _jsonVariant.printTo(json);
    auto httpCode = http.POST(json);

    // httpCode will be negative on error
    if (httpCode > 0)
    {
        if (httpCode == HTTP_CODE_OK)
        {
        }
    }
    else
    {
    }

    http.end();
}

void SenderClass::sendInfluxDB(String server, uint16_t port, String db, String name)
{
    HTTPClient http;

    String uri = "/write?db=";
    uri += db;

    // configure traged server and url
    http.begin(server, port, uri);
    http.addHeader("User-Agent", "iSpindel");
    http.addHeader("Connection", "close");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String msg;
    msg += "measurements,source=";
    msg += name;
    msg += " ";

    for (const auto &kv : _jsonVariant.as<JsonObject>())
    {
        msg += kv.key;
        msg += "=";
        msg += kv.value.as<String>();
        msg += ",";
    }
    msg.remove(msg.length() - 1);


    auto httpCode = http.POST(msg);

    // httpCode will be negative on error
    if (httpCode > 0)
    {
        if (httpCode == HTTP_CODE_OK)
        {
        }
    }
    else
    {
    }

    http.end();
}

void SenderClass::sendPrometheus(String server, uint16_t port, String job, String instance)
{
    HTTPClient http;
    
    // the path looks like /metrics/job/<JOBNAME>[/instance/<INSTANCENAME>]
    String uri = "/metrics/job/";
    uri += job;
    uri += "/instance/";
    uri += instance;

    // configure traged server and url
    http.begin(server, port, uri);
    http.addHeader("User-Agent", "iSpindel");
    http.addHeader("Connection", "close");
    http.addHeader("Content-Type", "text/plain");

    String msg;

    //Build up the data for the Prometheus Pushgateway
    //A gauge is a metric that represents a single numerical value that can arbitrarily go up and down.
    for (const auto &kv : _jsonVariant.as<JsonObject>())
    {
        msg += "# TYPE ";
        msg += kv.key;
        msg += " gauge\n";
        msg += "# HELP ";
        msg += kv.key;
        msg += " The approximate value of ";
        msg += kv.key;
        msg += ".\n";
        msg += kv.key;
        msg += " ";
        msg += kv.value.as<String>();
        msg += "\n";
    }

    auto httpCode = http.POST(msg);

    // httpCode will be 202 on success
    if (httpCode == 202)
    {
    }
    else
    {
    }

    http.end();
}

void SenderClass::sendUbidots(String token, String name)
{
    _jsonVariant.printTo(Serial);

    if (_client.connect(UBISERVER, 80))
    {

        String msg = F("POST /api/v1.6/devices/");
        msg += name;
        msg += "?token=";
        msg += token;
        msg += F(" HTTP/1.1\r\nHost: things.ubidots.com\r\nUser-Agent: GlyCon\r\nConnection: close\r\nContent-Type: application/json\r\nContent-Length: ");
        msg += _jsonVariant.measureLength();
        msg += "\r\n";

        _client.println(msg);

        _jsonVariant.printTo(_client);
        _client.println();
    }
    else
    {
    }

    int timeout = 0;
    while (!_client.available() && timeout < CONNTIMEOUT)
    {
        timeout++;
        delay(1);
    }
    while (_client.available())
    {
        char c = _client.read();
        Serial.write(c);
    }
    // currentValue = 0;
    _client.stop();
    delay(100); // allow gracefull session close
}

void SenderClass::sendFHEM(String server, uint16_t port, String name)
{
    _jsonVariant.printTo(Serial);

    if (_client.connect(server.c_str(), port))
    {

        String msg = String("GET /fhem?cmd.Test=set%20");
        msg += name;

        for (const auto &kv : _jsonVariant.as<JsonObject>())
        {
            msg += "%20";
            msg += kv.value.as<String>();
        }

        msg += F("&XHR=1 HTTP/1.1\r\nHost: ");
        msg += server;
        msg += ":";
        msg += port;
        msg += "\r\nUser-Agent: ";
        msg += name;
        msg += F("\r\nConnection: close\r\n");

        _client.println(msg);
    }
    else
    {
    }

    int timeout = 0;
    while (!_client.available() && timeout < CONNTIMEOUT)
    {
        timeout++;
        delay(1);
    }
    while (_client.available())
    {
        char c = _client.read();
        Serial.write(c);
    }
    // currentValue = 0;
    _client.stop();
    delay(100); // allow gracefull session close
}

void SenderClass::sendTCONTROL(String server, uint16_t port)
{
    _jsonVariant.printTo(Serial);

    if (_client.connect(server.c_str(), port))
    {

        String msg;

        for (const auto &kv : _jsonVariant.as<JsonObject>())
        {
            msg += kv.key;
            msg += ": ";
            msg += kv.value.as<String>();
            msg += " ";
        }
        msg.remove(msg.length() - 1);

        _client.println(msg);
    }
    else
    {
    }

    int timeout = 0;
    while (!_client.available() && timeout < CONNTIMEOUT)
    {
        timeout++;
        delay(1);
    }
    while (_client.available())
    {
        char c = _client.read();
        Serial.write(c);
    }
    // currentValue = 0;
    _client.stop();
    delay(100); // allow gracefull session close
}
