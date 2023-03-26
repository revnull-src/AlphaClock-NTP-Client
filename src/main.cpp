#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>   // https://github.com/arduino-libraries/NTPClient
#include <time.h>
#include <Timezone.h>    // https://github.com/JChristensen/Timezone
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <WiFiUdp.h>

// Configure your timezone rules as described here:
// https://github.com/JChristensen/Timezone#coding-timechangerules

// TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, -240}; // UTC - 4 hours
// TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, -300};  // UTC - 5 hours
TimeChangeRule usPDT = {"PDT", Second, Sun, Mar, 2, -420}; // UTC - 7 hours
TimeChangeRule usPST = {"PST", First, Sun, Nov, 2, -480};  // UTC - 8 hours
Timezone myTZ(usPDT, usPST);

char header = 0xff;
time_t rawtime, loctime;
uint64_t amicros = 0;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
WiFiManager wifiManager;

// Send the time mode payload to the alpha clock
void sendTime(int epoch)
{
    Serial.printf("%cST%d\n", header, epoch);
}

// Send word to alpha clock and pause for millisecond duration
void sendAlpha(String word, int timeout = 1)
{
    const char* cword = word.c_str();
    Serial.printf("%cA0%s_____\n", header, cword);
    delay(timeout);
}

// Set alpha clock to time mode
void timeMode()
{
    Serial.printf("%cMT----------\n", header);
}

// Notify user that WiFi setup is needed
void configModeCallback(WiFiManager *myWiFiManager)
{
    sendAlpha("SETUP");
}

void setup()
{
    Serial.begin(19200);
    delay(2000);
    Serial.println("\n*WM: AC5 NTP Client Starting...");

    // Fetch the SSID and password from EEPROM and try to
    // connect. If it fails to connect, start an access point
    // and go into a blocking loop awaiting configuration.
    wifiManager.setAPCallback(configModeCallback);
    wifiManager.setTimeout(180);
    if (!wifiManager.autoConnect("AC5_NTP_SETUP"))
    {
        sendAlpha("FAILD", 5000);
        sendAlpha("RETRY", 5000);
        ESP.restart();
        delay(5000);
    }
    else
    {
        // If we get this far we have connected to WiFi
        sendAlpha("WIFI ", 1000); // Let the user know we are checking NTP
        sendAlpha(" CONN", 3000);
        timeMode();               // Set the display to time mode
        timeClient.begin();       // Query NTP
    }
}

void loop()
{
    amicros = micros();
    if (timeClient.update())                 // NTP update
    {
        rawtime = timeClient.getEpochTime(); // Fetch NTP time
        loctime = myTZ.toLocal(rawtime);     // Calculate local time based on TZ
        sendTime(loctime);                   // Send time to Alpha Clock
        int remaining = 58000 - ((micros() - amicros) / 1000) - (second(loctime) * 1000);
        delay(remaining);                    // wait for end of the minute
    }
}
