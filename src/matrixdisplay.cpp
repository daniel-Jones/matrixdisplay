#define VERTICAL 1
#define ENA_SPRITE 1
#include <Arduino.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <string.h>
#include <stddef.h>
#include "time.h"
#include "index.h"
#undef VERTICAL
#ifdef VERTICAL
#include "font.h"
#endif

#include <Dictionary.h>
#include "esp_log.h"

#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <map>
#include <functional>

const char *ntpServer = "pool.ntp.org";
const char *tzone = "AEST-10AEDT,M10.1.0,M4.1.0/3";

Preferences preferences;
TaskHandle_t wifitask;
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 16
#define CS_PIN 5

#define NUM_MESSAGES 20
#define MSG_SIZE 200
JsonDocument jsonDocument;
JsonDocument jsonPumps;

// ap
const char *ssid = "dev test wip";
const char *password = "devtest123";

IPAddress dns1(1, 1, 1, 1);
IPAddress dns2(8, 8, 8, 8);

/* Put IP Address details */
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
WebServer server(80);

struct brightness
{
    unsigned int autoBrightness : 1;
    unsigned int brightness : 4;
	unsigned int autoBrightnessThreshold : 9; // max 512
    unsigned int RESERVED : 27;
};

struct
{
	int pos;
	struct brightness brightness;
	char cssid[256];
	char cpassword[256];
	char date[32];

} globalconf;
char pumpmsg[MSG_SIZE];
struct message
{
	bool enabled;
	bool invert;
	char msg[MSG_SIZE];
	textEffect_t effect1;
	textEffect_t effect2;
	int scrollpause;
	int speed;
};

bool wificonnected;
bool displayon;

void saveconfig();
void printLocalTime();
void loadconfig();
void defaultdata();
void handlecmd();

struct message messages[NUM_MESSAGES] = {0};

MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

#define LIGHT_SENSOR_PIN GPIO_NUM_34
#define LIGHT_BRIGHTNESS_MAX 400

void handleroot()
{
	server.send(200, "text/html", index_html);
}

Dictionary *pumpDic = new Dictionary();

void pumpstring()
{
	/*
	  pump1, pump2, pump3 ...
	  vs
	  thisvar=thisval, thatvar=thatval
	*/
	String body = server.arg("plain");
	deserializeJson(jsonPumps, body);
	String msgjson = jsonPumps["pump"];
	strncpy(pumpmsg, msgjson.c_str(), MSG_SIZE);
	printf("pump: %s\n", pumpmsg);
	printf("%s\n", body.c_str());
	pumpDic->jload(body.c_str());
	server.send(200, "text/html", pumpmsg);
}

void handlein()
{
	// save
	String body = server.arg("plain");
	deserializeJson(jsonDocument, body);
	int brightness = jsonDocument["brightness"];
	bool autoBrightness = jsonDocument["autoBrightness"];
	int autoBrightnessThreshold = jsonDocument["autoBrightnessThreshold"];
	String cssid = jsonDocument["cssid"];
	String cpassword = jsonDocument["cpassword"];
	String date = jsonDocument["datep"];
	globalconf.brightness.brightness = brightness;
	globalconf.brightness.autoBrightness = autoBrightness;
	globalconf.brightness.autoBrightnessThreshold = autoBrightnessThreshold;
	strncpy(globalconf.cssid, cssid.c_str(), 256);
	strncpy(globalconf.cpassword, cpassword.c_str(), 256);
	strncpy(globalconf.date, date.c_str(), 32);
	printf("Saving wifi: \"%s\" \"%s\"\n", globalconf.cssid, globalconf.cpassword);

	char mstring[15];
	char m[15];
	char menabled[15];
	char minvert[15];
	char meffect[15];
	char meffect2[15];
	char mpause[15];
	char mspeed[15];

	for (int i = 0; i < NUM_MESSAGES; i++)
	{
		sprintf(m, "m%d", i + 1);
		sprintf(menabled, "m%denabled", i + 1);
		sprintf(minvert, "m%dinvert", i + 1);
		sprintf(meffect, "m%deffect", i + 1);
		sprintf(meffect2, "m%deffect2", i + 1);
		sprintf(mpause, "m%dpause", i + 1);
		sprintf(mspeed, "m%dspeed", i + 1);
		sprintf(mstring, "m%d", i + 1);
		String msg = jsonDocument[mstring];
		messages[i].enabled = (jsonDocument[menabled] == "Enabled") ? true : false;
		strncpy(messages[i].msg, msg.c_str(), MSG_SIZE);
		if (jsonDocument[minvert] == "Invert")
		{
			messages[i].invert = 1;
		}
		else
		{
			messages[i].invert = 0;
		}
		String m1eff = jsonDocument[meffect];
		String m2eff = jsonDocument[meffect2];
		messages[i].effect1 = (textEffect_t)m1eff.toInt();
		messages[i].effect2 = (textEffect_t)m2eff.toInt();
		// if (jsonDocument[meffect] == "rtl"){messages[i].effect1 = PA_SCROLL_LEFT;} else {messages[i].effect1 = PA_SCROLL_RIGHT;}
		// if (jsonDocument[mpause] == "Pause"){messages[i].scrollpause = 2000;} else {messages[i].scrollpause = 0;}
		String p = jsonDocument[mpause];
		int pause = atoi(p.c_str());
		messages[i].scrollpause = pause;
		if (jsonDocument[mspeed] == "slow")
		{
			messages[i].speed = 60;
		}
		else if (jsonDocument[mspeed] == "medium")
		{
			messages[i].speed = 40;
		}
		else if (jsonDocument[mspeed] == "fast")
		{
			messages[i].speed = 20;
		}
		else
		{
			messages[i].speed = 40;
		}
		printf("message to save: %s\n", messages[i].msg);
	}

	saveconfig();

	server.send(200, "application/json", "{}");
}

char buffer[10800];
char wifistatus[32] = {0};
void handleconfigout()
{
	jsonDocument.clear();
	jsonDocument["brightness"] = globalconf.brightness.brightness;
	jsonDocument["autoBrightness"] = globalconf.brightness.autoBrightness;
	jsonDocument["autoBrightnessThreshold"] = globalconf.brightness.autoBrightnessThreshold;
	jsonDocument["cssid"] = globalconf.cssid;
	jsonDocument["cpassword"] = globalconf.cpassword;
	jsonDocument["datep"] = globalconf.date;

	if (WiFi.localIP())
	{
		strncpy(wifistatus, WiFi.localIP().toString().c_str(), 32);
	}
	else
	{
		strncpy(wifistatus, "Not connected", 32);
	}
	jsonDocument["wifistatus"] = wifistatus;

	char m[15];
	char menabled[15];
	char minvert[15];
	char meffect[15];
	char meffect2[15];
	char mpause[15];
	char mspeed[15];

	for (int i = 0; i < NUM_MESSAGES; i++)
	{
		sprintf(m, "m%d", i + 1);
		sprintf(menabled, "m%denabled", i + 1);
		sprintf(minvert, "m%dinvert", i + 1);
		sprintf(meffect, "m%deffect", i + 1);
		sprintf(meffect2, "m%deffect2", i + 1);
		sprintf(mpause, "m%dpause", i + 1);
		sprintf(mspeed, "m%dspeed", i + 1);

		jsonDocument[m] = messages[i].msg;
		jsonDocument[menabled] = messages[i].enabled;
		jsonDocument[minvert] = messages[i].invert;
		jsonDocument[meffect] = messages[i].effect1;	// TO INTERPRET
		jsonDocument[meffect2] = messages[i].effect2;	// TO INTERPRET
		jsonDocument[mpause] = messages[i].scrollpause; // TO INTERPRET
		jsonDocument[mspeed] = messages[i].speed;		// TO INTERPRET
	}

	serializeJson(jsonDocument, buffer);
	server.send(200, "application/json", buffer);
}

void handlenotfound()
{
}

void resetesp32()
{
	ESP.restart();
}

void synctime()
{
	printf("Syncing time\n");
	configTime(0, 0, ntpServer);
	setenv("TZ", tzone, 1);
	tzset();
	printLocalTime();
}

std::map<std::string, std::function<void()>> matrixCommands = {
    {"off", [&]() {
        printf("turning matrix off\n");
        displayon = false;
        myDisplay.displayShutdown(true);
    }},
    {"on", [&]() {
        printf("turning matrix on\n");
        displayon = true;
        myDisplay.displayShutdown(false);
    }},
    {"toggle", [&]() {
        printf("toggling matrix\n");
        displayon = !displayon;
        myDisplay.displayShutdown(!displayon);
    }}
};

void handlecmd()
{
	String body = server.arg("plain");
	deserializeJson(jsonPumps, body);
	const char *cmd = NULL;
	const char *value = NULL;
	JsonObject obj = jsonPumps.as<JsonObject>();
	for (JsonPair kv : obj)
	{
		cmd = kv.key().c_str();
		value = kv.value().as<const char *>();
		printf("%s : %s\n", cmd, value);
		// cmd handling
		if (strcmp(cmd, "matrix") == 0) {
			auto it = matrixCommands.find(value);
			if (it != matrixCommands.end()) {
				it->second();
			}
		} else if (strcmp(cmd, "restart") == 0) {
			resetesp32();
		}
	}
	server.send(200, "application/json", "");
}

void wifi(void *pvParameters)
{
	Serial.println("start wifi");
	WiFi.mode(WIFI_STA);
	Serial.println("set wifi mode\n");
	// web serverrouting
	server.on("/", handleroot);
	server.on("/msg", HTTP_POST, handlein);
	server.on("/config", handleconfigout);
	server.on("/reset", resetesp32);
	server.on("/pump", pumpstring);
	server.on("/cmd", handlecmd);
	server.begin();
	int retry = 0;
	WiFi.begin(globalconf.cssid, globalconf.cpassword);
	// WiFi.begin("nodwdw", "wqdqwr23er23");
	printf("wifi connect to: %s : %s\n", globalconf.cssid, globalconf.cpassword);
	Serial.println("Connecting to WiFi..");
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
		retry++;
		if (retry > 10)
		{
			printf("Too many first boot connect attempts to wifi.\nStarting AP mode..\n");
			WiFi.disconnect();
			WiFi.mode(WIFI_STA);
			WiFi.softAPConfig(local_ip, gateway, subnet);
			WiFi.softAP(ssid, password);
			IPAddress IP = WiFi.softAPIP();
			Serial.print("AP IP address: ");
			Serial.println(IP);
			break;
		}
	}
	wificonnected = WiFi.status() == WL_CONNECTED;
	if (retry <= 10 && wificonnected)
	{
		Serial.print("ESP32 IP on the WiFi network: ");
		Serial.println(WiFi.localIP());
		Serial.println(WiFi.dnsIP());
		synctime();
		ArduinoOTA.begin();
	}

	const unsigned long hourInterval = 3600000; // 1 hour in milliseconds
	unsigned long previousMillis = 0;
	unsigned long currentMillis = 0;
	while (1)
	{
		server.handleClient();
		if (wificonnected)
		{
			ArduinoOTA.handle();
		}
		// sync time hourly
		currentMillis = millis();
		if (wificonnected && currentMillis - previousMillis >= hourInterval)
		{
			previousMillis = currentMillis;
			synctime();
		}
		// delay(400);
	}
}

uint8_t scrollSpeed = 40; // default frame delay value
textEffect_t scrollEffect = PA_SCROLL_LEFT;
textPosition_t scrollAlign = PA_LEFT;
uint16_t scrollPause = 2000;

void printLocalTime()
{
	struct tm timeinfo;
	if (!getLocalTime(&timeinfo))
	{
		Serial.println("Failed to obtain time");
		return;
	}
	Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void setup()
{
	wificonnected = false;
	displayon = true;
	esp_log_level_set("*", ESP_LOG_VERBOSE);
	preferences.begin("matrixdisplay", false);
	globalconf.pos = 0;
	globalconf.brightness.brightness = 7;
	globalconf.brightness.autoBrightnessThreshold = 400;
	Serial.begin(115200);
	// Intialize the object
	myDisplay.begin();
	loadconfig();
#ifdef VERTICAL
	myDisplay.setFont(_fontVertical);
	myDisplay.setInvert(9);
#endif
	// Set the intensity (brightness) of the display (0-15)
	myDisplay.setIntensity(5);

	// Clear the display
	myDisplay.displayClear();
	xTaskCreatePinnedToCore(
		wifi,	   /* Task function. */
		"Task1",   /* name of task. */
		10000,	   /* Stack size of task */
		NULL,	   /* parameter of the task */
		1,		   /* priority of the task */
		&wifitask, /* Task handle to keep track of created task */
		0);		   /* pin task to core 0 */

	myDisplay.setTextAlignment(PA_LEFT);

	pinMode(LIGHT_SENSOR_PIN, INPUT);
}
void saveconfig()
{
	printf("saving matrixconfig of size: %ul\n", sizeof(messages));
	size_t ret = preferences.putBytes("matrixconfig", &messages, sizeof(messages));
	printf("actually wrote %u\n", ret);
	preferences.putBytes("globalconfig", &globalconf, sizeof(globalconf));
}

void loadconfig()
{
	int res = 0;
	res += preferences.getBytes("matrixconfig", &messages, sizeof(messages));
	res += preferences.getBytes("globalconfig", &globalconf, sizeof(globalconf));
	if (res == 0)
	{
		defaultdata();
	}
	// dont load pos
	globalconf.pos = 0;
}

void defaultdata()
{
	globalconf.pos = 0;
	globalconf.brightness.brightness = 4;
	globalconf.brightness.autoBrightness = false;
	globalconf.brightness.autoBrightnessThreshold = 400;
	strncpy(globalconf.cssid, "Gensokyo", 256);
	strncpy(globalconf.cpassword, "passwordhere", 256);
	strncpy(globalconf.date, "0", 32);
	messages[0].enabled = true;
	messages[0].invert = false;
	strncpy(messages[0].msg, "Not Setup", MSG_SIZE);
	messages[0].scrollpause = 5;
	messages[0].speed = 1;
}

void wipeconfig()
{
	preferences.clear();
	saveconfig();
}

#ifdef VERTICAL
void strrev(char *str)
{
	int i;
	int j;
	unsigned char a;
	unsigned int len = strlen((const char *)str);

	for (i = 0, j = len - 1; i < j; i++, j--)
	{
		a = str[i];
		str[i] = str[j];
		str[j] = a;
	}
}
#endif

char *string_replace(const char *source, size_t sourceSize, const char *substring, const char *with)
{
	char *substring_source = strstr(source, substring);
	if (substring_source == NULL)
	{
		return NULL;
	}

	memmove(
		substring_source + strlen(with),
		substring_source + strlen(substring),
		strlen(substring_source) - strlen(substring) + 1);

	memcpy(substring_source, with, strlen(with));
	return substring_source + strlen(with);
}

char msgbuff[MSG_SIZE];
char rtemp[15] = {0};
void nextmessage()
{
	if (!displayon) return;

	if (globalconf.pos >= NUM_MESSAGES)
	{
		globalconf.pos = 0;
	}
	if (messages[globalconf.pos].enabled == false)
	{
		globalconf.pos++;
		nextmessage();
		return;
	}

	memset(msgbuff, 0, MSG_SIZE);
	strcpy(msgbuff, messages[globalconf.pos].msg);
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	int autoBrightness = 0;
	int currentBrightness = 0;
	// do brightness now so it can be used in the replacements
	if (globalconf.brightness.autoBrightness)
	{
		int lightValue = constrain(analogRead(LIGHT_SENSOR_PIN), 0, globalconf.brightness.autoBrightnessThreshold);
		autoBrightness = map(lightValue, 0, globalconf.brightness.autoBrightnessThreshold, 0, 15);
		myDisplay.setIntensity(autoBrightness);
		currentBrightness = autoBrightness;
	} else
	{
		myDisplay.setIntensity(globalconf.brightness.brightness);
		currentBrightness = globalconf.brightness.brightness;
	}
	// Define the replacement patterns and their corresponding format strings
	const std::pair<const char*, const char*> replacements[] = {
		{"%date%", "%d/%m/%y"},
		{"%shortdate%", "%d %b"},
		{"%time12%", "%I:%M"},
		{"%time12s%", "%I:%M:%S"},
		{"%time24%", "%R"},
		{"%time24s%", "%T"},
		{"%pump%", pumpmsg},
		{"%brightness%", ""},
	};

	// Iterate over the replacements and perform the string replacements
	for (const auto& [pattern, format] : replacements) {
		if (strstr(msgbuff, pattern)) {
			if (strcmp(pattern, "%pump%") == 0) {
				string_replace(msgbuff, strlen(msgbuff), pattern, format);
			} else if (strcmp(pattern, "%brightness%") == 0) {
				snprintf(rtemp, sizeof(rtemp), "%d", currentBrightness);
				string_replace(msgbuff, strlen(msgbuff), pattern, rtemp);
			} else
			{
				strftime(rtemp, sizeof(rtemp), format, t);
				string_replace(msgbuff, strlen(msgbuff), pattern, rtemp);
			}
		}
	}

	/* replace pump messages */
	for (int i = 0; i < pumpDic->count(); i++)
	{
		if (strstr(msgbuff, pumpDic->key(i).c_str()))
		{
			printf("replace: %s\n", pumpDic->value(i));
			string_replace(msgbuff, strlen(msgbuff), (char *)pumpDic->key(i).c_str(), (char *)pumpDic->value(i).c_str());
		}
	}

	// doomsdate stuff
	struct tm doom = {0};
	strptime(globalconf.date, "%Y-%m-%d", &doom);

	time_t rawtime;
	struct tm *info;
	time(&rawtime);
	info = localtime(&rawtime);

	time_t t0 = mktime(info);
	time_t t1 = mktime(&doom);

	double diff = difftime(t1, t0); // Difference in seconds
	const char* timeUnits[] = {"%seconds%", "%minutes%", "%hours%", "%days%", "%weeks%"};
	const double unitDivisors[] = {1, 60, 3600, 86400, 604800};
	const int numUnits = sizeof(timeUnits) / sizeof(timeUnits[0]);

	for (int i = 0; i < numUnits; i++) {
		if (strstr(msgbuff, timeUnits[i])) {
			int value = static_cast<int>(diff / unitDivisors[i]);
			itoa(value, rtemp, 10);
			string_replace(msgbuff, strlen(msgbuff), timeUnits[i], rtemp);
		}
	}

#ifdef VERTICAL
	strrev(msgbuff);
#endif
	
	myDisplay.setInvert(messages[globalconf.pos].invert);
	scrollAlign = PA_CENTER;
	printf("displaying: %s\n", msgbuff);
	myDisplay.displayText(msgbuff, scrollAlign, messages[globalconf.pos].speed, messages[globalconf.pos].scrollpause * 1000, messages[globalconf.pos].effect1, messages[globalconf.pos].effect2);
	globalconf.pos++;
}

void loop()
{
	if (displayon && myDisplay.displayAnimate())
	{
		nextmessage();
		myDisplay.displayReset();
	}
}
