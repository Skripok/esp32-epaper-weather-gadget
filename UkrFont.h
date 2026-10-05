#pragma once
#include <Adafruit_GFX.h>

// Масив бітмапів для кирилиці та латини
const uint8_t UkrFontBitmaps[] PROGMEM = {
    // 0x20 Space
    0x00,
    // 0x21 '!'
    0x84, 0x21, 0x08, 0x00, 0x20,
    // 'І' (0x0406)
    0xFE, 0x10, 0x10, 0x10, 0x10, 0x10, 0xFE,
    // 'і' (0x0456)
    0x20, 0x00, 0x60, 0x20, 0x20, 0x20, 0x70,
    // 'Ї' (0x0407)
    0xAA, 0x00, 0xFE, 0x10, 0x10, 0x10, 0xFE,
    // 'ї' (0x0457)
    0x50, 0x00, 0x60, 0x20, 0x20, 0x20, 0x70,
    // 'Є' (0x0404)
    0x3C, 0x42, 0x80, 0xF0, 0x80, 0x42, 0x3C,
    // 'є' (0x0454)
    0x00, 0x3C, 0x42, 0x78, 0x40, 0x42, 0x3C,
    // 'Ч' (0x0427)
    0x82, 0x82, 0x82, 0x7E, 0x02, 0x02, 0x02,
    // 'ч' (0x0447)
    0x00, 0x82, 0x82, 0x7E, 0x02, 0x02, 0x02,
    // 'И' (0x0418)
    0x82, 0x86, 0x8A, 0x92, 0xA2, 0xC2, 0x82,
    // 'и' (0x0438)
    0x00, 0x82, 0x86, 0x8A, 0x92, 0xA2, 0xC2,
    // 'Й' (0x0419)
    0x44, 0x28, 0x82, 0x8A, 0x92, 0xA2, 0x82,
    // 'й' (0x0439)
    0x44, 0x00, 0x82, 0x8A, 0x92, 0xA2, 0x82,
    // '\'' (Апостроф)
    0x60, 0x20, 0x40, 0x00, 0x00, 0x00, 0x00
};

// Таблиця зсувів та параметрів гліфів
const GFXglyph UkrFontGlyphs[] PROGMEM = {
    {  0, 1, 1,  4, 0,  0 }, // 0: Space
    {  1, 3, 7,  4, 0, -6 }, // 1: !
    {  6, 7, 7,  8, 0, -6 }, // 2: І
    { 13, 3, 7,  4, 0, -6 }, // 3: і
    { 20, 7, 7,  8, 0, -6 }, // 4: Ї
    { 27, 3, 7,  4, 0, -6 }, // 5: ї
    { 34, 7, 7,  8, 0, -6 }, // 6: Є
    { 41, 7, 7,  8, 0, -6 }, // 7: є
    { 48, 7, 7,  8, 0, -6 }, // 8: Ч
    { 55, 7, 7,  8, 0, -6 }, // 9: ч
    { 62, 7, 7,  8, 0, -6 }, // 10: И
    { 69, 7, 7,  8, 0, -6 }, // 11: и
    { 76, 7, 7,  8, 0, -6 }, // 12: Й
    { 83, 7, 7,  8, 0, -6 }, // 13: й
    { 90, 2, 3,  3, 0, -6 }  // 14: '
};

// Декодер UTF-8 та попіксельний рендерер гліфів
template <typename DisplayType>
void printUkr(DisplayType& dev, int16_t x, int16_t y, const char* str) {
    int16_t curX = x;
    int16_t curY = y;

    while (*str) {
        uint32_t unicode = 0;
        uint8_t c = (uint8_t)*str;

        if (c < 0x80) {
            unicode = c;
            str++;
        } else if ((c & 0xE0) == 0xC0) {
            unicode = ((c & 0x1F) << 6) | ((uint8_t)*(str + 1) & 0x3F);
            str += 2;
        } else if ((c & 0xF0) == 0xE0) {
            unicode = ((c & 0x0F) << 12) | (((uint8_t)*(str + 1) & 0x3F) << 6) | ((uint8_t)*(str + 2) & 0x3F);
            str += 3;
        } else {
            str++;
            continue;
        }

        // Індекс гліфа в масиві
        int idx = -1;
        if (unicode == ' ') idx = 0;
        else if (unicode == '!') idx = 1;
        else if (unicode == 0x0406 || unicode == 0x0401) idx = 2; // І / Ґ
        else if (unicode == 0x0456 || unicode == 0x0451) idx = 3; // і / ґ
        else if (unicode == 0x0407) idx = 4; // Ї
        else if (unicode == 0x0457) idx = 5; // ї
        else if (unicode == 0x0404) idx = 6; // Є
        else if (unicode == 0x0454) idx = 7; // є
        else if (unicode == 0x0427) idx = 8; // Ч
        else if (unicode == 0x0447) idx = 9; // ч
        else if (unicode == 0x0418) idx = 10; // И
        else if (unicode == 0x0438) idx = 11; // и
        else if (unicode == 0x0419) idx = 12; // Й
        else if (unicode == 0x0439) idx = 13; // й
        else if (unicode == '\'' || unicode == '`') idx = 14;

        if (idx != -1) {
            GFXglyph g;
            memcpy_P(&g, &UkrFontGlyphs[idx], sizeof(GFXglyph));

            // Малювання бітмапу гліфа напряму в буфер
            for (uint8_t i = 0; i < g.height; i++) {
                for (uint8_t j = 0; j < g.width; j++) {
                    uint32_t bitOffset = (g.bitmapOffset * 8) + (i * g.width) + j;
                    uint8_t byteVal = pgm_read_byte(UkrFontBitmaps + (bitOffset / 8));
                    if (byteVal & (0x80 >> (bitOffset % 8))) {
                        dev.drawPixel(curX + j + g.xOffset, curY + i + g.yOffset, GxEPD_BLACK);
                    }
                }
            }
            curX += g.xAdvance;
        } else if (unicode < 0x80) {
            // Для звичайних англійських літер
            dev.setCursor(curX, curY - 6);
            dev.write((char)unicode);
            curX += 6;
        }
    }
}