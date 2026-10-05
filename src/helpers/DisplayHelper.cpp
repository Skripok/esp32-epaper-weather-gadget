#include "utils.h"
#include "DisplayHelper.h"
#include "icons.h"
#include <GxEPD2_BW.h>


// Гарантуємо наявність константи кольору для Adafruit_GFX
#ifndef GxEPD_BLACK
  #define GxEPD_BLACK 0x0000
#endif

// Інклюди шрифтів (підключені ТІЛЬКИ тут)
#include "../../fonts/cambriaUkr8.h"
#include "../../fonts/cambriaUkr12.h"
#include "../../fonts/cambriaUkr16.h"
#include "../../fonts/cambriaUkr18.h"
#include "../../fonts/cambriabUkr12.h"
#include "../../fonts/cambriabUkr16.h"

const GFXfont* DisplayHelper::getFont(FontSize size, FontStyle style) {
    if (style == FontStyle::BOLD) {
        switch (size) {
            case FontSize::SIZE_12: return &cambriabUkr12;
            case FontSize::SIZE_16: return &cambriabUkr16;
            default: return &cambriabUkr16;
        }
    } else {
        switch (size) {
            case FontSize::SIZE_8:  return &cambriaUkr8;
            case FontSize::SIZE_12: return &cambriaUkr12;
            case FontSize::SIZE_16: return &cambriaUkr16;
            case FontSize::SIZE_18: return &cambriaUkr18;
            default: return &cambriaUkr8;
        }
    }
}

void DisplayHelper::setFont(Adafruit_GFX& display, FontSize size, FontStyle style) {
    display.setFont(getFont(size, style));
}

void DisplayHelper::drawDivider(Adafruit_GFX& display, int16_t y) {
    display.drawLine(0, y, display.width(), y, GxEPD_BLACK);
}

// Перевантаження для float
void DisplayHelper::printTempWithCircle(Adafruit_GFX& display, int16_t x, int16_t y, float temp, 
                                       FontSize fontSize, FontStyle fontStyle, 
                                       int offsetUp, int radius) {
    printTempWithCircle(display, x, y, String(temp, 1), fontSize, fontStyle, offsetUp, radius);
}

// Перевантаження для String
void DisplayHelper::printTempWithCircle(Adafruit_GFX& display, int16_t x, int16_t y, const String& tempText, 
                                       FontSize fontSize, FontStyle fontStyle, 
                                       int offsetUp, int radius) {
    setFont(display, fontSize, fontStyle);
    display.setCursor(x, y);
    display.print(tempText);

    int degreeX = display.getCursorX() + 2;
    int degreeY = y - offsetUp;
    display.drawCircle(degreeX, degreeY, radius, GxEPD_BLACK);
    // Друкуємо літеру C одразу за кружечком
    display.setCursor(degreeX + radius + 3, y);
    display.print("C");
}

void DisplayHelper::drawWeatherIcon(Adafruit_GFX& display, int16_t x, int16_t y, WeatherCondition cond) {
    const uint8_t* icon = icons_sunny_day_32; // Значення за замовчуванням

    switch (cond) {
        case WeatherCondition::SUNNY:
            icon = icons_sunny_day_32;
            break;
        case WeatherCondition::NIGHT_CLEAR:
            // Поки використовуємо денну, як домовлялися
            icon = icons_sunny_day_32; 
            break;
        case WeatherCondition::PARTLY_CLOUDY:
            icon = icons_partly_cloudy_32;
            break;
        case WeatherCondition::CLOUDY:
            icon = icons_clouds_32;
            break;
        case WeatherCondition::RAIN:
            icon = icons_rain_32;
            break;
        case WeatherCondition::SNOW:
            icon = icons_snow_32;
            break;
        case WeatherCondition::STORM:
            icon = icons_storm_with_heavy_rain_32;
            break;
    }

    display.drawBitmap(x, y, icon, 32, 32, GxEPD_BLACK);
}

void DisplayHelper::drawFooter(Adafruit_GFX& display, const String& lastUpdated) {
    drawDivider(display, 265);
    display.setCursor(10, 285);
    setFont(display, FontSize::SIZE_8, FontStyle::REGULAR);

    String footerText = "wttr.in";
    if (lastUpdated.length() > 0) {
        footerText += String(Utf8win1251(" | Оновлено: ")) + lastUpdated;
    }

    display.print(footerText);
}

void DisplayHelper::printAdaptiveCondition(Adafruit_GFX& display, int16_t x, int16_t y, const String& conditionText, int16_t maxWidth) {
    String converted = Utf8win1251(conditionText);
    
    // Перевіряємо ширину тексту із шрифтом SIZE_16
    setFont(display, FontSize::SIZE_16, FontStyle::REGULAR);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(converted, x, y, &x1, &y1, &w, &h);

    // Якщо ширина більша за дозволену — зменшуємо шрифт до SIZE_12
    if (w > maxWidth) {
        setFont(display, FontSize::SIZE_12, FontStyle::REGULAR);
    }

    display.setCursor(x, y);
    display.print(converted);
}

void DisplayHelper::drawBattery(Adafruit_GFX& display, int16_t x, int16_t y, int percent) {
    // 1. Очищаємо область виводу білим кольором
    display.fillRect(x - 35, y - 2, 65, 16, GxEPD_WHITE);

    // 2. Вивід відсотків дефолтним шрифтом
    display.setFont(NULL); 
    display.setTextSize(1);
    display.setTextColor(GxEPD_BLACK);

    String pctStr = String(percent) + "%";
    int16_t textWidth = pctStr.length() * 6; 
    
    display.setCursor(x - textWidth - 3, y + 2);
    display.print(pctStr);

    // 3. Контур батареї (22x11 px)
    display.drawRect(x, y, 22, 11, GxEPD_BLACK);
    display.fillRect(x + 22, y + 3, 2, 5, GxEPD_BLACK); // Носик

    // 4. Секції заряду (4 секції по 4px, крок 5px = 19px загальної ширини)
    int numSegments = map(percent, 0, 100, 0, 4);
    for (int i = 0; i < numSegments; i++) {
        display.fillRect(x + 2 + (i * 5), y + 2, 4, 7, GxEPD_BLACK);
    }
}