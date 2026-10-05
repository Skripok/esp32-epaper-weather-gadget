#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <GxEPD2_BW.h>
#include <gdey/GxEPD2_420_GDEY042T81.h> 
#include <SPI.h>
#include <time.h>
#include <esp_task_wdt.h>
#include "esp_bt.h"
#include "src/enums/WeatherCondition.h"
#include "src/helpers/NetworkHelper.h"

#include "fonts/cambriaUkr8.h"
#include "fonts/cambriaUkr12.h"
#include "fonts/cambriaUkr16.h"
#include "fonts/cambriabUkr12.h"
#include "fonts/cambriabUkr16.h"
#include "fonts/cambriabUkr18.h"


#include "utils.h"
#include "src/helpers/DisplayHelper.h"

#define TIME_TO_SLEEP 1800
#define EPD_SCK   6
#define EPD_MOSI  7
#define EPD_CS    10
#define EPD_DC    4
#define EPD_RES   5
#define EPD_BUSY  3

GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display(
    GxEPD2_420_GDEY042T81(EPD_CS, EPD_DC, EPD_RES, EPD_BUSY)
);


WebServer server(80);
DNSServer dnsServer;
Preferences prefs;

struct CurrentWeather
{
    String temp;
    String humidity;
    String wind;
    WeatherCondition condition; 
};

struct ForecastDay
{
    String date;
    String tempMax;
    String tempMin;
    WeatherCondition condition; 
};

CurrentWeather current;
ForecastDay forecast[3];
const String LOCATION = "50.450,30.523";


void showStatus(const char* line1, const char* line2 = "") {
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);

        if (line1 != nullptr && strlen(line1) > 0) {
            display.setFont(&cambriabUkr18);
            display.setCursor(10, 80);
            String str1 = Utf8win1251(String(line1));
            display.print(str1);
        }

        if (line2 != nullptr && strlen(line2) > 0) {
            display.setFont(&cambriabUkr12);
            display.setCursor(10, 130);
            String str2 = Utf8win1251(String(line2));
            display.print(str2);
        }
    } while (display.nextPage());
}


String lastUpdatedStr = ""; 

void syncTime() {
    // Налаштовуємо таймзону України
    configTzTime("EET-2EEST,M3.5.0/3,M10.5.0/4", "pool.ntp.org", "time.nist.gov");
    
    struct tm timeinfo;
    int retry = 0;
    
    // Чекаємо успішної синхронізації часу (до 10 спроб по 500мс)
    while (!getLocalTime(&timeinfo) && retry < 10) {
        delay(500);
        retry++;
    }

    if (retry < 10) {
        char timeBuff[30];
        // Формат: 2026-08-22 14:30
        strftime(timeBuff, sizeof(timeBuff), "%Y-%m-%d %H:%M", &timeinfo);
        lastUpdatedStr = String(timeBuff);
    } else {
        lastUpdatedStr = "Помилка часу";
    }
}

bool fetchWeatherData()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        return false;
    }

    // Спочатку синхронізуємо час, щоб timeinfo був готовий
    syncTime();

    HTTPClient http;
    String url = "http://wttr.in/" + LOCATION + "?format=j1";
    http.begin(url);
    http.setUserAgent("curl/7.81.0"); // Додано User-Agent
    http.setTimeout(8000);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK)
    {
        String payload = http.getString();
        
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (error)
        {
            http.end();
            return false;
        }

        JsonObject current_condition = doc["current_condition"][0];
        
        current.temp = current_condition["temp_C"].as<String>(); 
        current.humidity = current_condition["humidity"].as<String>(); 
        current.wind = current_condition["windspeedKmph"].as<String>() + " км/год";
        
        String rawCond = current_condition["weatherDesc"][0]["value"].as<String>();
        current.condition = parseWeatherCondition(rawCond);

        // Парсимо 3 дні
        for (int i = 0; i < 3; i++)
        {
            JsonObject day = doc["weather"][i];
            forecast[i].date = day["date"].as<String>();
            forecast[i].tempMax = day["maxtempC"].as<String>();
            forecast[i].tempMin = day["mintempC"].as<String>();
            
            String dayCond = day["hourly"][4]["weatherDesc"][0]["value"].as<String>();
            forecast[i].condition = parseWeatherCondition(dayCond);
        }

        http.end();
        return true;
    }

    http.end();
    return false;
}

void renderDisplay()
{
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);

        // --- 1. ЗАГОЛОВОК ---
        DisplayHelper::setFont(display, FontSize::SIZE_18, FontStyle::BOLD);
        display.setCursor(10, 30);
        display.print(Utf8win1251("Моє місто"));
        DisplayHelper::drawDivider(display, 40);

        //1.1. Заряд батареї
        int batteryPct = getBatteryPercent(readBatteryVoltage()); 
        DisplayHelper::drawBattery(display, 340, 15, batteryPct);

        // --- 2. ПОТОЧНА ПОГОДА ---
        DisplayHelper::printTempWithCircle(display, 10, 70, current.temp, FontSize::SIZE_18, FontStyle::BOLD, 14, 3);
        DisplayHelper::drawWeatherIcon(display, 100, 42, current.condition);

        DisplayHelper::setFont(display, FontSize::SIZE_12, FontStyle::REGULAR);
        display.setCursor(10, 95);
        String details = "Вол: " + current.humidity + "% | Вітер: " + current.wind;
        display.print(Utf8win1251(details));

        DisplayHelper::drawDivider(display, 108);

        // --- 3. ПРОГНОЗ ПОГОДИ НА 3 ДНІ ---
        int16_t yLabel = 125;
        int16_t yDate  = 160;
        int16_t yTemp  = 190;
        int16_t yIcon  = 210;

        int16_t colX[3] = {5, 140, 270};

        // Отримуємо поточний день тижня з системного часу (NTP)
        struct tm timeinfo;
        int currentWday = 0;
        if (getLocalTime(&timeinfo))
        {
            currentWday = timeinfo.tm_wday; // 0..6
        }

        for (int i = 0; i < 3; i++)
        {
            int16_t x = colX[i];

            // 1. Динамічна назва дня тижня (+i днів від сьогодні)
            int dayIndex = (currentWday + i) % 7;
            String dayName = getDayOfWeekName(dayIndex);

            DisplayHelper::setFont(display, FontSize::SIZE_12, FontStyle::BOLD);
            display.setCursor(x, yLabel);
            display.print(Utf8win1251(dayName));

            // 2. Дата "ДД.ММ.РРРР"
            String rawDate = forecast[i].date;
            String formattedDate = rawDate;
            if (rawDate.length() >= 10)
            {
                String yyyy = rawDate.substring(0, 4);
                String mm   = rawDate.substring(5, 7);
                String dd   = rawDate.substring(8, 10);
                formattedDate = dd + "." + mm + "." + yyyy;
            }
            DisplayHelper::setFont(display, FontSize::SIZE_12, FontStyle::REGULAR);
            display.setCursor(x, yDate);
            display.print(formattedDate);

            // 3. Температура
            String tText = forecast[i].tempMin + "..." + forecast[i].tempMax;
            DisplayHelper::printTempWithCircle(display, x, yTemp, tText, FontSize::SIZE_12, FontStyle::BOLD, 10, 2);

            // 4. Іконка
            DisplayHelper::drawWeatherIcon(display, x + 44, yIcon, forecast[i].condition);
        }

        display.drawFastVLine(135, 108, 157, GxEPD_BLACK);
        display.drawFastVLine(265, 108, 157, GxEPD_BLACK);

        // --- 4. ФУТЕР ---
        DisplayHelper::drawFooter(display, lastUpdatedStr);

    } while (display.nextPage());
}

void startAP() {
    IPAddress apIP(192, 168, 4, 1);
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP("Weather-Gadget-AP");

    dnsServer.start(53, "*", apIP); // Перенаправлення всіх DNS запитів на ESP32

    showStatus("Налаштування Wi-Fi", "Точка: Weather-Gadget-AP\nIP: 192.168.4.1");

    auto handleRoot = []() {
        String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>"
                      "<style>body{font-family:sans-serif;padding:20px;} input{width:100%;padding:10px;margin:8px 0;box-sizing:border-box;}</style></head>body>"
                      "<h2>WiFi Settings</h2>"
                      "<form action='/save' method='POST'>"
                      "SSID:<br><input type='text' name='s' required><br>"
                      "Password:<br><input type='password' name='p'><br><br>"
                      "<input type='submit' value='Save & Connect' style='background-color:#4CAF50;color:white;border:none;'>"
                      "</form></body></html>";
        server.send(200, "text/html", html);
    };

    server.on("/", handleRoot);
    server.on("/generate_204", handleRoot); // Для Android Captive Portal
    server.on("/redirect", handleRoot);
    server.onNotFound(handleRoot); // Будь-який URL веде на форму

    server.on("/save", []() {
        String s = server.arg("s");
        String p = server.arg("p");
        
        prefs.begin("wifi", false);
        prefs.putString("ssid", s);
        prefs.putString("pass", p);
        prefs.end();

        server.send(200, "text/html", "<h2>Saved! Restarting...</h2>");
        delay(1000);
        ESP.restart();
    });

    server.begin();
    
    // Блокуючий цикл: працює ВІЧНО, поки користувач не збереже мережу
    while (true) {
        dnsServer.processNextRequest();
        server.handleClient();
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

void setup() 
{
    Serial.begin(115200);
    delay(1000);

    // Конфігуруємо WDT
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = 60000,
        .idle_core_mask = (1 << 0),
        .trigger_panic = false
    };
    esp_task_wdt_reconfigure(&twdt_config); // Використовуємо reconfigure замість init/add

    while (!Serial && millis() < 3000);

    Serial.println("\n--- START BOOT ---");
    Serial.flush();

    // 1. Зчитування батареї
    Serial.println("[1] Reading battery...");
    Serial.flush();

    float voltage = readBatteryVoltage();
    uint8_t percent = getBatteryPercent(voltage);
    Serial.printf("[1.1] Battery: %.2fV (%d%%)\n", voltage, percent);
    Serial.flush();

    // 2. Стабілізація та ініціалізація дисплея
    Serial.println("[2] Preparing E-Paper hardware...");
    Serial.flush();

    pinMode(EPD_BUSY, INPUT);
    delay(100);

    Serial.println("[2.1] Starting SPI bus...");
    Serial.flush();
    SPI.end();
    SPI.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS);

    Serial.println("[2.2] Initializing display driver...");
    Serial.flush();
    display.init(115200, true, 50, false);
    display.setRotation(0);

    Serial.println("[2.3] Display initialized successfully!");
    Serial.flush();

    // 3. Ініціалізація мережі
    Serial.println("[3] Initializing network connection...");
    Serial.flush();

    if (!NetworkHelper::init()) 
    {
        Serial.println("[3.1] Wi-Fi connection failed or AP mode started. Rendering fallback screen...");
        Serial.flush();

        display.firstPage();
        do 
        {
            display.fillScreen(GxEPD_WHITE);
            display.setTextColor(GxEPD_BLACK);
            display.setTextSize(2);
            display.setCursor(20, 40);
            display.print("Wi-Fi Not Connected");

            display.setTextSize(1);
            display.setCursor(20, 80);
            display.print("Connect to: Weather-Gadget-Setup");
            display.setCursor(20, 100);
            display.print("Open browser: 192.168.4.1");
        } while (display.nextPage());

        Serial.println("[3.2] Fallback screen updated. Powering off display.");
        Serial.flush();

        display.powerOff();
        
        // Вимикаємо WDT перед виходом/блокировкою в AP режим
        esp_task_wdt_delete(NULL);
        return; 
    }

    Serial.println("[3.3] Wi-Fi connected successfully!");
    Serial.flush();

    // 4. Отримання даних та оновлення екрана
    Serial.println("[4] Fetching weather data...");
    Serial.flush();

    if (fetchWeatherData()) 
    {
        Serial.println("[4.1] Weather data received. Updating E-Paper...");
        Serial.flush();

        renderDisplay();

        Serial.println("[5] E-Paper Updated Successfully!");
        Serial.flush();
    }
    else
    {
        Serial.println("[4.1-ERR] Failed to fetch weather data!");
        Serial.flush();
    }

    // 5. Завершення роботи та перехід у deep sleep
    Serial.println("[6] Disconnecting network and entering Deep Sleep...");
    Serial.flush();

    NetworkHelper::disconnect();

    // ОБОВ'ЯЗКОВО: видаляємо таску з WDT перед тим, як заснути!
    esp_task_wdt_delete(NULL);

    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * 1000000ULL);
    esp_deep_sleep_start();
}

void loop() 
{
    // Порожньо
}