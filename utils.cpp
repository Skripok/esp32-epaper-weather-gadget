#include "utils.h"

const UTF8Replace replacements[] = 
{
    {194, 176, 248}, // °
    {208, 129, 192}, // Ё
    {209, 145, 193}, // ё
    {208, 132, 194}, // Є
    {209, 148, 195}, // є
    {208, 134, 196}, // І
    {209, 150, 197}, // і
    {208, 135, 198}, // Ї
    {209, 151, 199}, // ї
    {210, 144, 200}, // Ґ
    {210, 145, 201}  // ґ
};

const int replacementsCount = sizeof(replacements) / sizeof(replacements[0]);

char* Utf8win1251(const char* source) 
{
    static char target[MAX_STRING + 1];
    target[0] = '\0';
    
    int i = 0, j = 0;
    while (source[i] && j < MAX_STRING) 
    {
        unsigned char first = source[i++];
        unsigned char n = first;

        if (first >= 127) 
        {
            unsigned char second = source[i++];
            bool replaced = false;
            for (int k = 0; k < replacementsCount; k++) 
            {
                if (replacements[k].firstByte == first && replacements[k].secondByte == second) 
                {
                    n = replacements[k].replacement;
                    replaced = true;
                    break;
                }
            }
            if (!replaced) 
            {
                n = second;
            }
        }

        target[j++] = n;
    }
    target[j] = '\0';
    return target;
}

char* Utf8win1251(const String& source) 
{
    return Utf8win1251(source.c_str());
}

WeatherCondition parseWeatherCondition(const String& rawCondition)
{
    String lower = rawCondition;
    lower.toLowerCase();

    if (lower.indexOf("thunder") != -1)
    {
        return WeatherCondition::STORM;
    }
    if (lower.indexOf("rain") != -1 || lower.indexOf("drizzle") != -1 || lower.indexOf("shower") != -1)
    {
        return WeatherCondition::RAIN;
    }
    if (lower.indexOf("snow") != -1 || lower.indexOf("blizzard") != -1 || lower.indexOf("sleet") != -1)
    {
        return WeatherCondition::SNOW;
    }
    if (lower.indexOf("partly") != -1)
    {
        return WeatherCondition::PARTLY_CLOUDY;
    }
    if (lower.indexOf("cloudy") != -1 || lower.indexOf("overcast") != -1)
    {
        return WeatherCondition::CLOUDY;
    }
    if (lower.indexOf("sunny") != -1 || lower.indexOf("clear") != -1)
    {
        return WeatherCondition::SUNNY;
    }

    return WeatherCondition::SUNNY;
}

String getWeatherConditionText(WeatherCondition condition)
{
    switch (condition)
    {
        case WeatherCondition::SUNNY:
            return "Ясно";
        case WeatherCondition::PARTLY_CLOUDY:
            return "Мінлива хмарність";
        case WeatherCondition::CLOUDY:
            return "Хмарно";
        case WeatherCondition::RAIN:
            return "Дощ";
        case WeatherCondition::SNOW:
            return "Сніг";
        case WeatherCondition::STORM:
            return "Гроза";
        default:
            return "Ясно";
    }
}

String getDayOfWeekName(int wday)
{
    switch (wday)
    {
        case 0: return "Неділя";
        case 1: return "Понеділок";
        case 2: return "Вівторок";
        case 3: return "Середа";
        case 4: return "Четвер";
        case 5: return "П'ятниця";
        case 6: return "Субота";
        default: return "";
    }
}


#include "utils.h"

float readBatteryVoltage()
{
    gpio_reset_pin((gpio_num_t)BATTERY_PIN);
    pinMode(BATTERY_PIN, INPUT);
    analogReadResolution(12);

    // Осереднення 16 замірів для придушення шуму АЦП
    uint32_t rawMvSum = 0;
    for (uint8_t i = 0; i < 16; ++i)
    {
        rawMvSum += analogReadMilliVolts(BATTERY_PIN);
        delay(1);
    }

    // Середня напруга на піні GPIO2 у вольтах
    float pinVoltage = (float)(rawMvSum >> 4) / 1000.0f;

    // Фізичний коефіцієнт дільника по результатах ручного вимірювання напруги з батареї і на GPI02 контакт
    const float V_MULTIPLIER = 1.6068f;

    // Реальна напруга акумулятора
    return pinVoltage * V_MULTIPLIER;
}

uint8_t getBatteryPercent(float voltage)
{
    uint16_t mv = (uint16_t)(voltage * 1000.0f);

    if (mv >= 4150) return 100;
    if (mv <= 3300) return 0;

    // Відкоригована нелінійна таблиця розряду Li-Ion під навантаженням
    static const uint16_t lookup[11] = {
        3300, // 0%   (нижче 3.3V контролер може працювати нестабільно)
        3500, // 10%
        3600, // 20%
        3660, // 30%
        3710, // 40%
        3750, // 50%  (3.73V тепер відповідатиме ~45%)
        3800, // 60%
        3850, // 70%
        3920, // 80%
        4020, // 90%
        4150  // 100%
    };

    for (uint8_t i = 0; i < 10; ++i)
    {
        if (mv >= lookup[i] && mv <= lookup[i + 1])
        {
            return (i * 10) + ((mv - lookup[i]) * 10 / (lookup[i + 1] - lookup[i]));
        }
    }

    return 0;
}
