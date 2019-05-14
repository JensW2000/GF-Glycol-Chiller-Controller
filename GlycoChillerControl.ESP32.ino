/*
	Name:       GlycoChillerControl.ino
	Created:	01.06.2018 07:57:30
	Author:     Jens

*/

#include <Esp.h>

//#include <sstream.h>
#include <string.h>
#include <U8x8lib.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#include "Sender.h"

#include <Metro.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "Html.h"
#include <Preferences.h>


Preferences settings;

#define OWRESOLUTION 12  

#define OW0_PIN 26
#define OW1_PIN 15
#define OW2_PIN 27
#define OW3_PIN 19
const int OW_PIN[4]{ OW0_PIN, OW1_PIN, OW2_PIN, OW3_PIN };

#define RELAIS0_PIN 4 
#define RELAIS1_PIN 16
#define RELAIS2_PIN 17
#define RELAIS3_PIN 18

#define SDA_PIN 21
#define SCL_PIN 22

const ulong MEASURE_INTERVAL = 30000;
const ulong UBIDOTS_INTERVAL = 60000;
const ulong DSERROR_INTERVAL = 5000;
const ulong DISPLAY_INTERVAL = 2000;
const ulong LOG_TEMP_INTERVAL = 15000;
const ulong WIFI_CHECK_INTERVAL = 300000;

const byte DNS_PORT = 53;

String AP_NAME;
String AP_PASS;
String SSID;
String PASS;

bool PORT_ACTIVE[4] = { false,false,false,false };
bool PORT_ENABLED[4] = { false,false,false,false };

float currentTemp[4] = { -127 ,-127 ,-127 ,-127 };
float targetTemp[4] = { 16, 16, 16, 16 };
float hysterese[4] = { 0, 0, 0, 0 };
String sudName[4];

bool ubidotsEnabled = false;
String ubidotsToken;
ulong ubidotsIntervall = 1;

bool configChanged = false;
bool configApModeChanged = false;

uint8_t displayPort = 0;

OneWire OW0(OW0_PIN);
OneWire OW1(OW1_PIN);
OneWire OW2(OW2_PIN);
OneWire OW3(OW3_PIN);
OneWire OW[4]{ OW0,OW1,OW2,OW3 };

DallasTemperature DS18B20_0(&OW0);
DallasTemperature DS18B20_1(&OW1);
DallasTemperature DS18B20_2(&OW2);
DallasTemperature DS18B20_3(&OW3);
DallasTemperature DS18B20[4]{ DS18B20_0, DS18B20_1, DS18B20_2, DS18B20_3 };

DeviceAddress dsDeviceAddress[4];

Metro getTempMetro0(MEASURE_INTERVAL);
Metro getTempMetro1(MEASURE_INTERVAL);
Metro getTempMetro2(MEASURE_INTERVAL);
Metro getTempMetro3(MEASURE_INTERVAL);
Metro getTempMetro[4]{ getTempMetro0, getTempMetro1, getTempMetro2, getTempMetro3 };

Metro logTempMetro(LOG_TEMP_INTERVAL);
Metro displayMetro(DISPLAY_INTERVAL);
Metro ubidotsMetro(UBIDOTS_INTERVAL);
Metro wiFiCheckMetro(WIFI_CHECK_INTERVAL);

WebServer webServer(80);
DNSServer dnsServer;

U8X8_SH1106_128X64_NONAME_SW_I2C oled(/* clock=*/ SCL_PIN, /* data=*/ SDA_PIN, /* reset=*/ U8X8_PIN_NONE);   // OLEDs without Reset of the Display
//U8X8_SH1106_128X64_NONAME_HW_I2C oled(/* reset=*/ U8X8_PIN_NONE, /* clock=*/ SCL_PIN, /* data=*/ SDA_PIN);   // OLEDs without Reset of the Display

uint8_t GradC_TileUpperLine[16] = { 6,9,9,6,0,252,254,3 ,3,3,3,3,3,3,14,12 };
uint8_t GradC_TileBottomLine[16] = { 0,0,0,0,0,15,31,48 ,48,48,48,48,48,28,12 };
uint8_t GradC_Tile_small[8] = { 3, 3, 126, 129, 129, 129, 129, 66 };
uint8_t Arrow_TileUpperLine[16] = { 128,128,128,128,128,128,128,128, 144,144,160,160,192,192,128,128 };
uint8_t Arrow_TileBottomLine[16] = { 1,1,1,1,1,1,1,1, 9,9,5,5,3,3,1,1 };

uint8_t Equal_TileUpperLine[16] = { 0,0,0,32,32,32,32,32, 32,32,32,32,32,0,0,0 };
uint8_t Equal_TileBottomLine[16] = { 0,0,0,1,1,1,1,1, 1,1,1,1,1,0,0,0 };
uint8_t Approx_TileUpperLine[16] = { 0,64,32,144,144,144,32,64, 128,0,0,0,0,0,128,0 };
uint8_t Approx_TileBottomLine[16] = { 0,2,1,0,0,0,1,2, 4,9,18,18,18,9,4,0 };
uint8_t DummyTileLine[16]{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
uint8_t TempType_TileUpperLine[16];
uint8_t TempType_TileBottomLine[16];


uint8_t Line_Tile[128] = {
	0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,
	0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,
	0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,
	0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,
	0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,
	0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,
	0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,
	0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,
	0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,
	0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,
	0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,
	0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,
	0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,
	0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,
	0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,
	0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8
};


//interface

void getTemperature(bool force = false, uint8_t port = 10);
const String getStatusHTML();
const String getSettingsHTML();
//implementation

void initPINs()
{
	pinMode(OW0_PIN, OUTPUT);
	pinMode(OW1_PIN, OUTPUT);
	pinMode(OW2_PIN, OUTPUT);
	pinMode(OW3_PIN, OUTPUT);

	pinMode(RELAIS0_PIN, OUTPUT);
	pinMode(RELAIS1_PIN, OUTPUT);
	pinMode(RELAIS2_PIN, OUTPUT);
	pinMode(RELAIS3_PIN, OUTPUT);
}

void initSettings()
{
	settings.begin("settings");
	targetTemp[0] = settings.getFloat("targetTemp0", 16);
	targetTemp[1] = settings.getFloat("targetTemp1", 16);
	targetTemp[2] = settings.getFloat("targetTemp2", 16);
	targetTemp[3] = settings.getFloat("targetTemp3", 16);
	hysterese[0] = settings.getFloat("hysterese0", .1);
	hysterese[1] = settings.getFloat("hysterese1", .1);
	hysterese[2] = settings.getFloat("hysterese2", .1);
	hysterese[3] = settings.getFloat("hysterese3", .1);
	PORT_ENABLED[0] = settings.getBool("portEnabled0", false);
	PORT_ENABLED[1] = settings.getBool("portEnabled1", false);
	PORT_ENABLED[2] = settings.getBool("portEnabled2", false);
	PORT_ENABLED[3] = settings.getBool("portEnabled3", false);
	sudName[0] = settings.getString("sudName0", "");
	sudName[1] = settings.getString("sudName1", "");
	sudName[2] = settings.getString("sudName2", "");
	sudName[3] = settings.getString("sudName3", "");
	SSID = settings.getString("SSID", "");
	PASS = settings.getString("PASS", "");
	AP_NAME = settings.getString("AP_NAME", "GlyCon");
	AP_PASS = settings.getString("AP_PASS", "chillmybeer!");
	ubidotsEnabled = settings.getBool("ubidotsEnabled", false);
	ubidotsToken = settings.getString("ubidotsToken", "");
	ubidotsIntervall = settings.getULong("ubidotsIntervall", 1);
}

void initOneWire(uint8_t port = 10)
{

	for (uint8_t i = 0; i < 4; i++)
	{
		if (port == i || port == 10)
		{

			digitalWrite(OW_PIN[i], LOW);
			DS18B20[i].begin();
			DS18B20[i].setWaitForConversion(true);
			if (!DS18B20[i].getAddress(dsDeviceAddress[i], 0)) Serial.print(F("DS18B20 ")); Serial.print(i); Serial.println(F(": NO ADDRESS"));
			DS18B20[i].setResolution(dsDeviceAddress[i], OWRESOLUTION);
			DS18B20[i].setWaitForConversion(false);
		}
		if (port == i)
		{
			getTemperature(true);
			break;
		}
		else getTemperature(true, port);
	}
}

void initOled()
{
	delay(2000);
	oled.begin();
	oled.setPowerSave(0);
}

void initWifiManager()
{
	WiFi.mode(WIFI_MODE_APSTA);
	WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
	WiFi.softAP(AP_NAME.c_str(), AP_PASS.c_str());
	dnsServer.start(DNS_PORT, "*", IPAddress(192, 168, 4, 1));

	WiFi.begin(SSID.c_str(), PASS.c_str());

	uint retries = 0;
	while (!(WiFi.status() == WL_CONNECTED || retries > 100))
	{
		delay(50);
		yield();
		retries += 1;
		if ((WiFi.status() == WL_CONNECT_FAILED)) break;
	}
	Serial.println(WiFi.localIP());

	webServer.on("/", []() {
		webServer.send(200, "text/html", getStatusHTML());
	});
	webServer.on("/index.html", [&]() {
		webServer.send(200, "text/html", getStatusHTML());
	});
	webServer.on("/hotspot-detect.html", [&]() {
		webServer.send(200, "text/html", getStatusHTML());
	});
	webServer.on("/settings.html", [&]() {
		if (webServer.hasArg("saveAction"))
		{
			Serial.println("--------SAVE-------");

			PORT_ENABLED[0] = webServer.hasArg("port0Enabled");
			PORT_ENABLED[1] = webServer.hasArg("port1Enabled");
			PORT_ENABLED[2] = webServer.hasArg("port2Enabled");
			PORT_ENABLED[3] = webServer.hasArg("port3Enabled");
			if (webServer.hasArg("port0SudName"))
				sudName[0] = webServer.arg("port0SudName");
			if (webServer.hasArg("port1SudName"))
				sudName[1] = webServer.arg("port1SudName");
			if (webServer.hasArg("port2SudName"))
				sudName[2] = webServer.arg("port2SudName");
			if (webServer.hasArg("port3SudName"))
				sudName[3] = webServer.arg("port3SudName");
			if (webServer.hasArg("port0Temp"))
				targetTemp[0] = webServer.arg("port0Temp").toFloat();
			if (webServer.hasArg("port1Temp"))
				targetTemp[1] = webServer.arg("port1Temp").toFloat();
			if (webServer.hasArg("port2Temp"))
				targetTemp[2] = webServer.arg("port2Temp").toFloat();
			if (webServer.hasArg("port3Temp"))
				targetTemp[3] = webServer.arg("port3Temp").toFloat();
			if (webServer.hasArg("port0Hysterese"))
				hysterese[0] = webServer.arg("port0Hysterese").toFloat();
			if (webServer.hasArg("port1Hysterese"))
				hysterese[1] = webServer.arg("port1Hysterese").toFloat();
			if (webServer.hasArg("port2Hysterese"))
				hysterese[2] = webServer.arg("port2Hysterese").toFloat();
			if (webServer.hasArg("port3Hysterese"))
				hysterese[3] = webServer.arg("port3Hysterese").toFloat();
			ubidotsEnabled = webServer.hasArg("ubidotsEnabled");
			if (webServer.hasArg("ubidotsToken"))
				ubidotsToken = webServer.arg("ubidotsToken");
			if (webServer.hasArg("ubidotsIntervall"))
				ubidotsIntervall = webServer.arg("ubidotsIntervall").toInt();


			settings.putFloat("targetTemp0", targetTemp[0]);
			settings.putFloat("targetTemp1", targetTemp[1]);
			settings.putFloat("targetTemp2", targetTemp[2]);
			settings.putFloat("targetTemp3", targetTemp[3]);
			settings.putFloat("hysterese0", hysterese[0]);
			settings.putFloat("hysterese1", hysterese[1]);
			settings.putFloat("hysterese2", hysterese[2]);
			settings.putFloat("hysterese3", hysterese[3]);
			settings.putBool("portEnabled0", PORT_ENABLED[0]);
			settings.putBool("portEnabled1", PORT_ENABLED[1]);
			settings.putBool("portEnabled2", PORT_ENABLED[2]);
			settings.putBool("portEnabled3", PORT_ENABLED[3]);
			settings.putString("sudName0", sudName[0]);
			settings.putString("sudName1", sudName[1]);
			settings.putString("sudName2", sudName[2]);
			settings.putString("sudName3", sudName[3]);
			settings.putBool("ubidotsEnabled", ubidotsEnabled);
			settings.putULong("ubidotsIntervall", ubidotsIntervall);
			settings.putString("ubidotsToken", ubidotsToken);
			ubidotsMetro.interval(UBIDOTS_INTERVAL * ubidotsIntervall);
			ubidotsMetro.reset();

			Serial.println(PORT_ENABLED[0]);
			Serial.println(PORT_ENABLED[1]);
			Serial.println(PORT_ENABLED[2]);
			Serial.println(PORT_ENABLED[3]);
			Serial.println(sudName[0]);
			Serial.println(sudName[1]);
			Serial.println(sudName[2]);
			Serial.println(sudName[3]);
			Serial.println(targetTemp[0]);
			Serial.println(targetTemp[1]);
			Serial.println(targetTemp[2]);
			Serial.println(targetTemp[3]);
			Serial.println(hysterese[0]);
			Serial.println(hysterese[1]);
			Serial.println(hysterese[2]);
			Serial.println(hysterese[3]);
			Serial.println(ubidotsEnabled);
			Serial.println(ubidotsToken);
			Serial.println(ubidotsIntervall);

		}

		webServer.send(200, "text/html", getSettingsHTML());
	});
	webServer.on("/wifi.html", [&]() {
		if (webServer.hasArg("saveAction"))
		{
			Serial.println("--------SAVE-WIFI------");
			if (webServer.hasArg("ssid") && webServer.hasArg("pass"))
			{
				SSID = webServer.arg("ssid");
				PASS = webServer.arg("pass");
				AP_NAME = webServer.arg("ap_name");
				AP_PASS = webServer.arg("ap_pass");
				settings.putString("SSID", SSID);
				settings.putString("PASS", PASS);
				settings.putString("AP_NAME", AP_NAME);
				settings.putString("AP_PASS", AP_PASS);
				WiFi.disconnect();
				WiFi.softAPdisconnect();
				WiFi.begin(webServer.arg("ssid").c_str(), webServer.arg("pass").c_str());
				WiFi.softAP(AP_NAME.c_str(), AP_PASS.c_str());

			}
		}
		webServer.send(200, "text/html", getWiFiHTML());
	});


	webServer.onNotFound([]() {
		webServer.send(200, "text/html", "ERR 404:" + webServer.uri() + "<br />");
	});

	webServer.begin();
}

void setup()
{
	Serial.begin(115200);
	initPINs();
	initOled();
	oled.setFont(u8x8_font_pxplusibmcgathin_f);
	oled.draw2x2String(0, 0, "Setup");
	initSettings();
	initOneWire();
	initWifiManager();
	oled.draw2x2String(0, 3, "done ...");
}

void setPins()
{
	digitalWrite(RELAIS0_PIN, PORT_ENABLED[0] && PORT_ACTIVE[0]);
	digitalWrite(RELAIS1_PIN, PORT_ENABLED[1] && PORT_ACTIVE[1]);
	digitalWrite(RELAIS2_PIN, PORT_ENABLED[2] && PORT_ACTIVE[2]);
	digitalWrite(RELAIS3_PIN, PORT_ENABLED[3] && PORT_ACTIVE[3]);
}

bool isDS18B20_Error(uint8_t port = 10)
{
	switch (port)
	{
	case 0:
		return (PORT_ENABLED[0] && (currentTemp[0] == -127 || currentTemp[0] == 85));
	case 1:
		return (PORT_ENABLED[1] && (currentTemp[1] == -127 || currentTemp[1] == 85));
	case 2:
		return (PORT_ENABLED[2] && (currentTemp[2] == -127 || currentTemp[2] == 85));
	case 3:
		return (PORT_ENABLED[3] && (currentTemp[3] == -127 || currentTemp[3] == 85));
	default:
		return
			(PORT_ENABLED[0] && (currentTemp[0] == -127 || currentTemp[0] == 85)) ||
			(PORT_ENABLED[1] && (currentTemp[1] == -127 || currentTemp[1] == 85)) ||
			(PORT_ENABLED[2] && (currentTemp[2] == -127 || currentTemp[2] == 85)) ||
			(PORT_ENABLED[3] && (currentTemp[3] == -127 || currentTemp[3] == 85));
	}

}

void getTemperature(bool force, uint8_t port)
{

	for (uint8_t i = 0; i < 4; i++)
	{
		if (getTempMetro[i].check() || (force && (port == 10 || port == i)))
		{
			DS18B20[i].setWaitForConversion(false);
			DS18B20[i].requestTemperatures();
			DS18B20[i].setWaitForConversion(true);
		}

		if (DS18B20[i].isConversionComplete())
		{
			currentTemp[i] = DS18B20[i].getTempCByIndex(0);
			getTempMetro[i].interval(MEASURE_INTERVAL);
			if (isDS18B20_Error(i))
			{
				digitalWrite(OW_PIN[i], LOW);
				OW[i].reset();
				initOneWire(i);
				getTempMetro[i].interval(DSERROR_INTERVAL);
			}
		}
	}
}

void setPortStatus()
{
	for (uint8_t i = 0; i < 4; i++)
	{
		if (!isDS18B20_Error(i))
		{
			PORT_ACTIVE[i] = (PORT_ACTIVE[i] && currentTemp[i] >= targetTemp[i] - (hysterese[i])) ||
				currentTemp[i] > targetTemp[i];
		}
		else
		{
			PORT_ACTIVE[i] = false;
		}
	}
}

const String floatToString(float aFloat, int aPrecision)
{
	char valueString[5];
	dtostrf(aFloat, 2, aPrecision, valueString);
	return String(valueString);
}

const String getDisplayTemp(uint8_t aPort, float aFloat, int aPrecision, bool longText)
{
	if (!PORT_ENABLED[aPort]) return " -- ";
	if (aFloat == -127) return longText ? "DS fehlt" : "n.c.";
	if (aFloat == 85) return longText ? "DS Init" : "n.i.";
	return floatToString(aFloat, aPrecision);
}


const String boolToAnAus(bool aBool)
{
	return aBool ? String("an") : String("aus");
}
const String getPortStatusStr(uint8_t aPort)
{
	if (!PORT_ENABLED[aPort]) return "DISABLED";
	if (isDS18B20_Error(aPort)) return "DS ERROR";
	return "OK";
}

void checkWiFi()
{
	if (WiFi.status() != WL_CONNECTED)
	{
		WiFi.disconnect();
		WiFi.softAPdisconnect();
		WiFi.begin(webServer.arg("ssid").c_str(), webServer.arg("pass").c_str());
		WiFi.softAP(AP_NAME.c_str(), AP_PASS.c_str());
	}
}

void updateDisplay()
{
	if (!PORT_ENABLED[0] && !PORT_ENABLED[1] && !PORT_ENABLED[2] && !PORT_ENABLED[3])
	{
		oled.clearDisplay();
		oled.draw2x2UTF8(0, 0, "Port 1-4");
		oled.drawUTF8(0, 3, "inaktiv");
		oled.drawUTF8(0, 5, AP_NAME.c_str());
		oled.drawUTF8(0, 7, WiFi.localIP().toString().c_str());
		return;
	}
	displayPort += 1;
	if (displayPort == 4 && PORT_ENABLED[0] && PORT_ENABLED[1] && PORT_ENABLED[2] && PORT_ENABLED[3])
	{
		oled.clearDisplay();
		oled.draw2x2UTF8(0, 0, "IP CONF");
		oled.drawTile(0, 2, 16, Line_Tile);
		oled.drawUTF8(0, 5, AP_NAME.c_str());
		oled.drawUTF8(0, 7, WiFi.localIP().toString().c_str());
		return;
	}

	if (displayPort > 3) displayPort = 0;

	if (PORT_ENABLED[displayPort])
		displayMetro.interval(DISPLAY_INTERVAL * 2);
	else
		displayMetro.interval(DISPLAY_INTERVAL);
	displayMetro.reset();

	oled.clearDisplay();

	if (PORT_ENABLED[displayPort])
	{
		oled.draw2x2UTF8(0, 0, (String("Port ") + String(displayPort + 1)).c_str());
		oled.drawTile(0, 2, 16, Line_Tile);
		oled.drawUTF8(0, 3, ("Ist : " + String((currentTemp[displayPort] < 10) ? " " : "") + getDisplayTemp(displayPort, currentTemp[displayPort], 1, true)).c_str());
		if (!isDS18B20_Error(displayPort))
			oled.drawTile(11, 3, 1, GradC_Tile_small);
		oled.drawUTF8(0, 5, ("Soll: " + String((targetTemp[displayPort] < 10) ? " " : "") + getDisplayTemp(displayPort, targetTemp[displayPort], 1, true)).c_str());
		oled.drawTile(11, 5, 1, GradC_Tile_small);
		oled.drawUTF8(0, 7, sudName[displayPort].c_str());
	}
	else
	{
		oled.draw2x2UTF8(0, 0, (String("Port ") + String(displayPort + 1)).c_str());
		oled.drawTile(0, 2, 16, Line_Tile);
		oled.drawUTF8(0, 3, "inaktiv");
		oled.drawUTF8(0, 5, AP_NAME.c_str());
		oled.drawUTF8(0, 7, WiFi.localIP().toString().c_str());
	}
}

void uploadData()
{

	ubidotsMetro.interval(UBIDOTS_INTERVAL * ubidotsIntervall);
	ubidotsMetro.reset();

	if (!PORT_ENABLED[0] && !PORT_ENABLED[1] && !PORT_ENABLED[2] && !PORT_ENABLED[3])
		return;

	SenderClass sender;
	if (PORT_ENABLED[0] && !isDS18B20_Error(0))
	{
		sender.add("currentTemp1", currentTemp[0]);
		sender.add("targetTemp1", targetTemp[0]);
	}
	if (PORT_ENABLED[1] && !isDS18B20_Error(1))
	{
		sender.add("currentTemp2", currentTemp[1]);
		sender.add("targetTemp2", targetTemp[1]);
	}
	if (PORT_ENABLED[2] && !isDS18B20_Error(2))
	{
		sender.add("currentTemp3", currentTemp[2]);
		sender.add("targetTemp3", targetTemp[2]);
	}
	if (PORT_ENABLED[3] && !isDS18B20_Error(3))
	{
		sender.add("currentTemp4", currentTemp[3]);
		sender.add("targetTemp4", targetTemp[3]);
	}

	if (sender.hasData())
		sender.sendUbidots(ubidotsToken, AP_NAME);
}

void loop()
{

	getTemperature();

	setPortStatus();

	if (logTempMetro.check())
	{
		Serial.print("T0: "); Serial.print(currentTemp[0]); Serial.print(" / Pump ON: "); Serial.print(PORT_ACTIVE[0] && PORT_ENABLED[0]); Serial.print(" / ENABLED: "); Serial.println(PORT_ENABLED[0]);
		Serial.print("T1: "); Serial.print(currentTemp[1]); Serial.print(" / Pump ON: "); Serial.print(PORT_ACTIVE[1] && PORT_ENABLED[1]); Serial.print(" / ENABLED: "); Serial.println(PORT_ENABLED[1]);
		Serial.print("T2: "); Serial.print(currentTemp[2]); Serial.print(" / Pump ON: "); Serial.print(PORT_ACTIVE[2] && PORT_ENABLED[2]); Serial.print(" / ENABLED: "); Serial.println(PORT_ENABLED[2]);
		Serial.print("T3: "); Serial.print(currentTemp[3]); Serial.print(" / Pump ON: "); Serial.print(PORT_ACTIVE[3] && PORT_ENABLED[3]); Serial.print(" / ENABLED: "); Serial.println(PORT_ENABLED[3]);
		Serial.println();
	}

	setPins();
	if (displayMetro.check())
		updateDisplay();

	if (wiFiCheckMetro.check())
		checkWiFi();

	if (ubidotsEnabled && ubidotsMetro.check())
	{
		uploadData();
	}

	dnsServer.processNextRequest();
	webServer.handleClient();

	delay(20);

}

String getHtmlStyle()
{
	return
		"    <style>\n"
		"        input {\n"
		"            padding: 5px;\n"
		"            font-size: 1em;\n"
		"            width: 94%;\n"
		"            margin: 3px;\n"
		"        }\n"
		"\n"
		"        body {\n"
		"            text-align: center;\n"
		"            font-family: verdana;\n"
		"            background-color: black;\n"
		"            color: white;\n"
		"        }\n"
		"\n"
		"        h3 {\n"
		"            padding:3px;\n"
		"        }\n"
		"        a {\n"
		"            color: #1fa3ec;\n"
		"        }\n"
		"\n"
		"        button {\n"
		"            border: 0;\n"
		"            margin-top: 1px;\n"
		"            margin-bottom: 2px;\n"
		"            border-radius: 0.3em;\n"
		"            background-color: #1fa3ec;\n"
		"            color: #fff;\n"
		"            line-height: 2.4em;\n"
		"            font-size: 1.2em;\n"
		"            width: 100%;\n"
		"            display: block;\n"
		"        }\n"
		"\n"
		"        .q {\n"
		"            float: right;\n"
		"        }\n"
		"\n"
		"        .s {\n"
		"            display: inline-block;\n"
		"            width: 14em;\n"
		"            overflow: hidden;\n"
		"            text-overflow: ellipsis;\n"
		"            white-space: nowrap;\n"
		"        }\n"
		"\n"
		"        #wl {\n"
		"            line-height: 1.5em;\n"
		"        }\n"
		"\n"
		"        table {\n"
		"			 border:1px solid #1fa3ec;"
		"            border-spacing: 0;\n"
		"            width: 100%;\n"
		"        }\n"
		"\n"
		"            table .th {\n"
		"                background-color: #1fa3ec;\n"
		"                text-align: center;\n"
		"            }\n"
		"\n"
		"            table .ti {\n"
		"				 width: 10px; \n"
		"            }\n"
		"\n"
		"        .tdTemp {\n"
		"            width: 20%;\n"
		"            text-align: right;\n"
		"        }\n"
		"\n"
		"        .tdPumpe {\n"
		"            text-align: center;\n"
		"        }\n"
		"\n"
		"        td {\n"
		"            padding-right: 3px;\n"
		"            padding-left: 3px;\n"
		"        }\n"
		"\n"
		"        .box {\n"
		"            border: 1px solid 1fa3ec;\n"
		"            background-color:#333;\n"
		"            margin-bottom:10px;\n"
		"        }\n"
		"        .box h3 {\n"
		"            margin:0;\n"
		"            color:#1fa3ec;\n"
		"            padding: 3px;\n"
		"        }\n"
		"        label {\n"
		"            border: 0;\n"
		"            padding: 3px;\n"
		"            display: block;\n"
		"        }\n"
		"\n"
		"            label input {\n"
		"                width:auto;\n"
		"                display:inline;\n"
		"                margin-right:10px;\n"
		"				 padding:0;\n"
		"            }\n"
		"    </style>\n";

}

const String getStatusHTML()
{
	String html;
	html +=
		"<!DOCTYPE html>"
		"<html lang = \"de\">"
		"<head>"
		"<meta charset = \"utf-8\" />\n"
		"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\" />\n" +
		getHtmlStyle() +
		"</head>\n"
		"<body>\n"
		"    <h2 id=\"h2\">Glycol Chiller<br />Status</h2>\n"
		"    <div style='text-align:left;display:inline-block;width:300px;padding:5px'>\n"
		"        <table>\n"
		"            <tr class=\"th\">\n"
		"                <td class=\"ti th\"></td>\n"
		"                <td class=\"th\">\n"
		"                    Soll\n"
		"                </td>\n"
		"                <td class=\"th\">\n"
		"                    Ist\n"
		"                </td>\n"
		"                <td class=\"th\">\n"
		"                    Pumpe\n"
		"                </td>\n"
		"                <td class=\"th\">\n"
		"                    Status\n"
		"                </td>\n"
		"            </tr>\n"
		"            <tr>\n"
		"                <td class=\"ti th\">1</td>\n"
		"                <td class=\"tdTemp\">"
		+ getDisplayTemp(0, targetTemp[0], 1, false) +
		"</td>\n"
		"                <td class=\"tdTemp\">"
		+ getDisplayTemp(0, currentTemp[0], 1, false) +
		"</td>\n"
		"                <td class=\"tdPumpe\">"
		+ boolToAnAus(PORT_ACTIVE[0] && PORT_ENABLED[0]) +
		"</td>\n"
		"                <td>"
		+ getPortStatusStr(0) +
		"</td>\n"
		"            </tr>\n"
		"            <tr>\n"
		"                <td class=\"ti th\">2</td>\n"
		"                <td class=\"tdTemp\">"
		+ getDisplayTemp(1, targetTemp[1], 1, false) +
		"</td>\n"
		"                <td class=\"tdTemp\">"
		+ getDisplayTemp(1, currentTemp[1], 1, false) +
		"</td>\n"
		"                <td class=\"tdPumpe\">"
		+ boolToAnAus(PORT_ACTIVE[1] && PORT_ENABLED[1]) +
		"</td>\n"
		"                <td>"
		+ getPortStatusStr(1) +
		"</td>\n"
		"            </tr>\n"
		"            <tr>\n"
		"                <td class=\"ti th\">3</td>\n"
		"                <td class=\"tdTemp\">"
		+ getDisplayTemp(2, targetTemp[2], 1, false) +
		"</td>\n"
		"                <td class=\"tdTemp\">"
		+ getDisplayTemp(2, currentTemp[2], 1, false) +
		"</td>\n"
		"                <td class=\"tdPumpe\">"
		+ boolToAnAus(PORT_ACTIVE[2] && PORT_ENABLED[2]) +
		"</td>\n"
		"                <td>"
		+ getPortStatusStr(2) +
		"</td>\n"
		"            </tr>\n"
		"            <tr>\n"
		"                <td class=\"ti th\">4</td>\n"
		"                <td class=\"tdTemp\">"
		+ getDisplayTemp(3, targetTemp[3], 1, false) +
		"</td>\n"
		"                <td class=\"tdTemp\">"
		+ getDisplayTemp(3, currentTemp[3], 1, false) +
		"</td>\n"
		"                <td class=\"tdPumpe\">"
		+ boolToAnAus(PORT_ACTIVE[3] && PORT_ENABLED[3]) +
		"</td>\n"
		"                <td>"
		+ getPortStatusStr(3) +
		"</td>\n"
		"            </tr>\n"
		"        </table>\n"
		"        <table>\n"
		"            <tr class=\"th\">\n"
		"                <td class=\"ti th\"></td>\n"
		"                <td class=\"th\">Sud</td>\n"
		"            </tr>\n"
		"            <tr>\n"
		"                <td class=\"ti th\">1</td>\n"
		"                <td>" + sudName[0] + "</td>\n"
		"            </tr>\n"
		"            <tr>\n"
		"                <td class=\"ti th\">2</td>\n"
		"                <td>" + sudName[1] + "</td>\n"
		"            </tr>\n"
		"            <tr>\n"
		"                <td class=\"ti th\">3</td>\n"
		"                <td>" + sudName[2] + "</td>\n"
		"            </tr>\n"
		"            <tr>\n"
		"                <td class=\"ti th\">4</td>\n"
		"                <td>" + sudName[3] + "</td>\n"
		"            </tr>\n"
		"        </table>\n"
		"<br />"
		"<div style=\"color: #1fa3ec;\">WiFi IP: </div>\n" +
		(String)WiFi.localIP().toString() +
		"        <h6>\n"
		"            letztes Update vor\n"
		"            <span id=\"ut\"></span>\n"
		"            <span id=\"status\"></span>\n"
		"        </h6>\n"
		"        <hr />\n"
		"        <form style=\"margin:0; padding:0;\" method=\"get\" action=\"/index.html\">\n"
		"            <button type=\"submit\" id=\"btnUpdate\" formaction=\"/index.html\">Aktualisieren</button>\n"
		"            <button type=\"submit\" id=\"btnConfig\" formaction=\"/settings.html\">Konfiguration</button>\n"
		"            <button type=\"submit\" id=\"btnWiFi\" formaction=\"/wifi.html\" formmethod=\"post\">WiFi Einstellungen</button>\n"
		"        </form>\n"
		" 	</div>\n"
		"\n"
		"  <script>\n"
		"        function g(i) { return document.getElementById(i) };\n"
		"        var statusRequest, updateTime;\n"
		"        updateTime = 0;\n"
		"        function displayTime(seconds)\n"
		"        {\n"
		"            var numdays = Math.floor(seconds / 86400);\n"
		"            var numhours = Math.floor((seconds % 86400) / 3600);\n"
		"            var numminutes = Math.floor(((seconds % 86400) % 3600) / 60);\n"
		"            var numseconds = ((seconds % 86400) % 3600) % 60;\n"
		"            var s = \"\";\n"
		"            if (numdays > 0) s += numdays + ((numdays == 1) ? \"Tag, \" : \"Tage, \");\n"
		"            if (numhours > 0 || numdays > 0) s += ((numdays > 0 && numhours < 10) ? \"0\" +numhours : numhours) + \":\";\n"
		"            if (numminutes > 0 || numhours > 0 || numdays > 0) s += (((numdays > 0 || numhours > 0) && numminutes < 10) ? \"0\" + numminutes : numminutes);\n"
		"            if (numhours == 0 && numdays == 0) \n"
		"            {\n"
		"                if (numminutes > 0)\n"
		"                {\n"
		"                    s += \":\";\n"
		"                    s += (numseconds < 10) ? \"0\" + numseconds : numseconds;\n"
		"                    s += (numseconds == 1) ? \" Minute\" : \" Minuten\"\n"
		"                }\n"
		"                else s += numseconds + (numseconds == 1 ? \" Sekunde.\" : \" Sekunden.\");\n"
		"            }\n"
		"            if (numhours > 0 || numdays > 0) \n"
		"            {\n"
		"                s += numdays > 0 ? (numdays > 1 ? \" Tage\" : \" Tag\") : \" Stunden.\";\n"
		"            }\n"
		"            return s;\n"
		"        }\n"
		"        setInterval(function () { g(\"ut\").innerHTML = displayTime(++updateTime); }, 1000);\n"
		"    </script>"
		"</body>\n"
		"</html>"
		;

	return html;
}

const String getSettingsHTML()
{
	String html;
	html +=
		"<!DOCTYPE html>"
		"<html lang = \"de\">"
		"<head>"
		"<meta charset = \"utf-8\" />\n"
		"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\" />\n" +
		getHtmlStyle() +
		"</head>\n"
		"<body>\n"
		"    <h2 id=\"h2\">Glycol Chiller<br />Konfiguration</h2>\n"
		"    <div style='text-align:left;display:inline-block;width:300px;padding:5px'>\n"
		"        <form style=\"margin:0; padding:0;\" method=\"get\" action=\"/settings.html\">\n"
		"            <div class=\"box\">\n"
		"                <h3>Port 1</h3>\n"
		"                <label><input type=\"checkbox\" name=\"port0Enabled\" " +
		String(PORT_ENABLED[0] ? "checked=\"checked\"" : "") +
		"/>Port benutzt</label>\n"
		"                <label>Sudname</label>\n"
		"                <input type=\"text\" name=\"port0SudName\" maxlength=\"16\"  value=\"" +
		sudName[0] +
		"\"/><br />\n"
		"                <label>Temperatur (-1 - 25 &deg;C)</label>\n"
		"                <input type=\"number\" max=\"25\" min=\"-1\" name=\"port0Temp\" required step=\"0.1\" value=\"" +
		floatToString(targetTemp[0], 1) +
		"\"/> \n"
		"                <label>Hysterese (0-2,0 &deg;C)</label>\n"
		"                <input type=\"number\" max=\"2\" min=\"0.0\" name=\"port0Hysterese\" required step=\"0.1\" value=\"" +
		floatToString(hysterese[0], 1) +
		"\"/> \n"
		"            </div>\n"
		"            <div class=\"box\">\n"
		"                <h3>Port 2</h3>\n" +
		"                <label><input type=\"checkbox\" name=\"port1Enabled\" " +
		String(PORT_ENABLED[1] ? "checked=\"checked\"" : "") +
		"/>Port benutzt</label>\n"
		"                <label>Sudname</label>\n"
		"                <input type=\"text\" name=\"port1SudName\" maxlength=\"16\"  value=\"" +
		sudName[1] +
		"\"/><br />\n"
		"                <label>Temperatur (-1 - 25 &deg;C)</label>\n"
		"                <input type=\"number\" max=\"25\" min=\"-1\" name=\"port1Temp\" required step=\"0.1\" value=\"" +
		floatToString(targetTemp[1], 1) +
		"\"/> \n"
		"                <label>Hysterese (0-2,0 &deg;C)</label>\n"
		"                <input type=\"number\" max=\"2\" min=\"0.0\" name=\"port1Hysterese\" required step=\"0.1\" value=\"" +
		floatToString(hysterese[1], 1) +
		"\"/> \n"
		"            </div>\n"
		"            <div class=\"box\">\n"
		"                <h3>Port 3</h3>\n"
		"                <label><input type=\"checkbox\" name=\"port2Enabled\" " +
		String(PORT_ENABLED[2] ? "checked=\"checked\"" : "") +
		"/>Port benutzt</label>\n"
		"                <label>Sudname</label>\n"
		"                <input type=\"text\" name=\"port2SudName\" maxlength=\"16\"  value=\"" +
		sudName[2] +
		"\"/><br />\n"

		"                <label>Temperatur (-1 - 25 &deg;C)</label>\n"
		"                <input type=\"number\" max=\"25\" min=\"-1\" name=\"port2Temp\" required step=\"0.1\" value=\"" +
		floatToString(targetTemp[2], 1) +
		"\"/> \n"
		"                <label>Hysterese (0-2,0 &deg;C)</label>\n"
		"                <input type=\"number\" max=\"2\" min=\"0.0\" name=\"port2Hysterese\" required step=\"0.1\" value=\"" +
		floatToString(hysterese[2], 1) +
		"\"/> \n"
		"            </div>\n"
		"            <div class=\"box\">\n"
		"                <h3>Port 4</h3>\n"
		"                <label><input type=\"checkbox\" name=\"port3Enabled\" " +
		String(PORT_ENABLED[3] ? "checked=\"checked\"" : "") +
		"/>Port benutzt</label>\n"
		"                <label>Sudname</label>\n"
		"                <input type=\"text\" name=\"port3SudName\" maxlength=\"16\"  value=\"" +
		sudName[3] +
		"\"/><br />\n"

		"                <label>Temperatur (-1 - 25 &deg;C)</label>\n"
		"                <input type=\"number\" max=\"25\" min=\"-1\" name=\"port3Temp\" required step=\"0.1\" value=\"" +
		floatToString(targetTemp[3], 1) +
		"\"/> \n"
		"                <label>Hysterese (0-2,0 &deg;C)</label>\n"
		"                <input type=\"number\" max=\"2\" min=\"0.0\" name=\"port3Hysterese\" required step=\"0.1\" value=\"" +
		floatToString(hysterese[3], 1) +
		"\"/> \n"
		"            </div>\n"
		"            <div class=\"box\">\n"
		"                <h3>Ubidots</h3>\n"
		"                <label><input type=\"checkbox\" name=\"ubidotsEnabled\" " +
		String(ubidotsEnabled ? "checked=\"checked\"" : "") +
		"/>Daten senden</label>\n"
		"                <label>Token</label>\n"
		"                <input type=\"text\" name=\"ubidotsToken\" maxlength=\"50\" value=\"" +
		ubidotsToken +
		"\" /><br />\n"
		"                <label>Intervall (1-60 Minuten)</label>\n"
		"                <input type=\"number\" max=\"60\" min=\"0\" name=\"ubidotsIntervall\" required step=\"1\" value=\"" +
		String(ubidotsIntervall) +
		"\" />\n"
		"            </div>"
		"            <hr />\n"
		"<input type=\"hidden\" name =\"saveAction\" value=\"save\" />\n"
		"            <button type=\"submit\" id=\"btnSave\" value=\"saveAction\" formaction=\"/settings.html\">Speichern</button>\n"
		"        </form>\n"
		"        <hr />\n"
		"        <form style=\"margin:0; padding:0;\" method=\"get\" action=\"/index.html\">\n"
		"            <button type=\"submit\" id=\"btnUpdate\" formaction=\"/index.html\">Status</button>\n"
		"            <button type=\"submit\" id=\"btnWiFi\" formaction=\"/wifi.html\" formmethod=\"post\">WiFi Einstellungen</button>\n"
		"        </form>\n"
		"    </div>\n"
		"\n"
		"</body>\n"
		"</html>"
		;

	return html;
}

const String getWiFiHTML()
{
	String html;
	html +=
		"<!DOCTYPE html>"
		"<html lang = \"de\">"
		"<head>"
		"<meta charset = \"utf-8\" />\n"
		"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\" />\n" +
		getHtmlStyle() +
		"</head>\n"
		"<body>\n"
		"    <h2 id=\"h2\">Glycol Chiller<br />WiFi</h2>\n"
		"    <div style='text-align:left;display:inline-block;width:300px;padding:5px'>\n"
		"        <form style=\"margin:0; padding:0;\" method=\"get\" action=\"/settings.html\">\n"
		"            <div class=\"box\">\n"
		"                <label>SSID</label>\n"
		"                <input type=\"text\" name=\"ssid\" maxlength=\"16\"  value=\"" +
		String(SSID) +
		"\"/><br />\n"
		"                <label>Passwort</label>\n"
		"                <input type=\"password\" name=\"pass\" maxlength=\"16\"  value=\"" +
		String(PASS) +
		"\"/><br />\n"
		"            </div>\n"
		"            <div class=\"box\">\n"
		"                <label>AP NAME</label>\n"
		"                <input type=\"text\" name=\"ap_name\" maxlength=\"16\"  value=\"" +
		String(AP_NAME) +
		"\"/><br />\n"
		"                <label>AP Passwort</label>\n"
		"                <input type=\"password\" name=\"ap_pass\" maxlength=\"16\"  value=\"" +
		String(AP_PASS) +
		"\"/><br />\n"
		"            </div>\n"
		"            <hr />\n"
		"<input type=\"hidden\" name =\"saveAction\" value=\"save\" />\n"
		"            <button type=\"submit\" id=\"btnSave\" value=\"saveAction\" formaction=\"/wifi.html\">Speichern</button>\n"
		"        </form>\n"
		"        <hr />\n"
		"        <form style=\"margin:0; padding:0;\" method=\"get\" action=\"/index.html\">\n"
		"            <button type=\"submit\" id=\"btnUpdate\" formaction=\"/index.html\">Status</button>\n"
		"        </form>\n"
		"    </div>\n"
		"\n"
		"</body>\n"
		"</html>"
		;

	return html;
}


