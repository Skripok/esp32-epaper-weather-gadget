#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "src/enums/WeatherCondition.h"

#define MAX_STRING 270
#define BATTERY_PIN 2
// Оголошуємо константи як constexpr, щоб вони були доступні у всіх файлах, де включено utils.h
constexpr float R1 = 100000.0f;
constexpr float R2 = 100000.0f;
constexpr float V_MULTIPLIER = (R1 + R2) / R2; // 2.0f

struct UTF8Replace
{
    uint8_t firstByte;
    uint8_t secondByte;
    uint8_t replacement;
};

extern const UTF8Replace replacements[];
extern const int replacementsCount;

char* Utf8win1251(const char* source);
char* Utf8win1251(const String& source);

// Конвертує сирий англійський рядок від API безпосередньо в Enum
WeatherCondition parseWeatherCondition(const String& rawCondition);

// Повертає локалізований український текст за Enum
String getWeatherConditionText(WeatherCondition condition);

// Повертає назву дня тижня українською мовою (0 = Неділя, 1 = Понеділок і т.д.)
String getDayOfWeekName(int wday);

float readBatteryVoltage();

uint8_t getBatteryPercent(float voltage);
