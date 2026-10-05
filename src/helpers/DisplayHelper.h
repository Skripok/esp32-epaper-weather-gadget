#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "../enums/WeatherCondition.h"

enum class FontSize {
    SIZE_8,
    SIZE_12,
    SIZE_16,
    SIZE_18
};


enum class FontStyle {
    REGULAR,
    BOLD
};


extern const GFXfont cambriaUkr8;
extern const GFXfont cambriaUkr12;
extern const GFXfont cambriaUkr16;
extern const GFXfont cambriabUkr12;
extern const GFXfont cambriabUkr16;
extern const GFXfont cambriabUkr18;


class DisplayHelper {
public:
// Повертає вказівник на шрифт за параметрами
    static const GFXfont* getFont(FontSize size = FontSize::SIZE_8, FontStyle style = FontStyle::REGULAR);

    // Допоміжний метод встановлення шрифту
    static void setFont(Adafruit_GFX& display, FontSize size = FontSize::SIZE_8, FontStyle style = FontStyle::REGULAR);

    static void drawDivider(Adafruit_GFX& display, int16_t y);
    // Для float значення
    static void printTempWithCircle(Adafruit_GFX& display, int16_t x, int16_t y, float temp, 
                                   FontSize fontSize = FontSize::SIZE_16, FontStyle fontStyle = FontStyle::BOLD, 
                                   int offsetUp = 10, int radius = 3);

    // Для String значення (нове перевантаження)
    static void printTempWithCircle(Adafruit_GFX& display, int16_t x, int16_t y, const String& tempText, 
                                   FontSize fontSize = FontSize::SIZE_16, FontStyle fontStyle = FontStyle::BOLD, 
                                   int offsetUp = 10, int radius = 3);
    static void drawWeatherIcon(Adafruit_GFX& display, int16_t x, int16_t y, WeatherCondition cond);
    static void drawFooter(Adafruit_GFX& display, const String& lastUpdated);
    static void printAdaptiveCondition(Adafruit_GFX& display, int16_t x, int16_t y, const String& conditionText, int16_t maxWidth);
    static void drawBattery(Adafruit_GFX& display, int16_t x, int16_t y, int percent);
};