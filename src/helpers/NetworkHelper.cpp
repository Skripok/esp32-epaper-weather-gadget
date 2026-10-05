#include "NetworkHelper.h"

#define LED_PIN 8 // Синій світлодіод на ESP32-C3 SuperMini (Active LOW)

Preferences NetworkHelper::preferences;
WebServer NetworkHelper::server(80);
DNSServer NetworkHelper::dnsServer;

String NetworkHelper::ssid = "";
String NetworkHelper::password = "";

bool NetworkHelper::init(uint32_t timeoutMs) 
{
    // Налаштовуємо пін світлодіода
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH); // Вимкнено (active LOW)

    preferences.begin("wifi_config", true);
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
    preferences.end();

    if (ssid.length() == 0) 
    {
        startAP();
        return false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    uint32_t startAttemptTime = millis();
    uint32_t lastBlinkTime = 0;
    bool ledState = false;

    while (WiFi.status() != WL_CONNECTED) 
    {
        if (millis() - startAttemptTime >= timeoutMs) 
        {
            digitalWrite(LED_PIN, HIGH); // Вимикаємо LED при помилці
            WiFi.disconnect(true);
            startAP();
            return false;
        }

        // Блимання кожні 150 мс під час пошуку Wi-Fi
        if (millis() - lastBlinkTime >= 150) 
        {
            lastBlinkTime = millis();
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState ? LOW : HIGH);
        }

        delay(10);
    }

    // Успішно підключено — вимикаємо LED, щоб не споживав струм
    digitalWrite(LED_PIN, HIGH);
    return true;
}

void NetworkHelper::startAP() 
{
    WiFi.mode(WIFI_AP);
    IPAddress apIP(192, 168, 4, 1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP("Weather-Gadget-Setup");

    dnsServer.start(53, "*", apIP);

    server.on("/", handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.onNotFound(handleRoot);
    server.begin();

    // У режимі точки доступу світлодіод горить постійно
    digitalWrite(LED_PIN, LOW); 

    uint32_t apStartTime = millis();
    while (true) 
    {
        dnsServer.processNextRequest();
        server.handleClient();
        delay(10);

        if (millis() - apStartTime > 300000) 
        {
            disconnect();
            esp_deep_sleep_start();
        }
    }
}

void NetworkHelper::handleRoot() 
{
    String html = "<!DOCTYPE html><html><head>"
                  "<meta charset='UTF-8'>"
                  "<meta name='viewport' content='width=device-width, initial-scale=1'>"
                  "<title>Weather Gadget Wi-Fi Setup</title>"
                  "<style>body{font-family:Arial,sans-serif;margin:20px;} input{width:100%;padding:10px;margin:8px 0;box-sizing:border-box;}"
                  "input[type=submit]{background-color:#4CAF50;color:white;border:none;cursor:pointer;padding:12px;font-size:16px;}</style></head><body>"
                  "<h2>Налаштування Wi-Fi Мережі</h2>"
                  "<form action='/save' method='POST'>"
                  "<label>SSID (Назва мережі):</label><input type='text' name='ssid' required>"
                  "<label>Пароль:</label><input type='password' name='password'>"
                  "<input type='submit' value='Зберегти та перезавантажити'>"
                  "</form></body></html>";
    
    server.send(200, "text/html; charset=utf-8", html);
}

void NetworkHelper::handleSave() 
{
    if (server.hasArg("ssid")) 
    {
        String newSsid = server.arg("ssid");
        String newPassword = server.arg("password");

        preferences.begin("wifi_config", false);
        preferences.putString("ssid", newSsid);
        preferences.putString("password", newPassword);
        preferences.end();

        String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'></head><body>"
                      "<h2>Дані збережено!</h2><p>Гаджет перезавантажується...</p></body></html>";
        
        server.send(200, "text/html; charset=utf-8", html);
        delay(2000);
        ESP.restart();
    }
}

bool NetworkHelper::syncTime(const char* ntpServer, long gmtOffset_sec, int daylightOffset_sec) 
{
    if (WiFi.status() != WL_CONNECTED) return false;

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    struct tm timeinfo;
    uint32_t startAttemptTime = millis();
    while (!getLocalTime(&timeinfo)) 
    {
        if (millis() - startAttemptTime >= 5000) return false;
        delay(100);
    }
    return true;
}

void NetworkHelper::resetSettings() 
{
    preferences.begin("wifi_config", false);
    preferences.clear();
    preferences.end();
}

void NetworkHelper::disconnect() 
{
    digitalWrite(LED_PIN, HIGH); // Вимикаємо LED перед сном
    dnsServer.stop();
    server.stop();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(50);
}