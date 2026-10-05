#ifndef NETWORK_HELPER_H
#define NETWORK_HELPER_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <time.h>

class NetworkHelper 
{
private:
    static Preferences preferences;
    static WebServer server;
    static DNSServer dnsServer;
    
    static String ssid;
    static String password;

    static void handleRoot();
    static void handleSave();
    static void startAP();

public:
    static bool init(uint32_t timeoutMs = 15000);
    static bool syncTime(const char* ntpServer = "pool.ntp.org", long gmtOffset_sec = 7200, int daylightOffset_sec = 3600);
    static void resetSettings();
    static void disconnect();
};

#endif // NETWORK_HELPER_H