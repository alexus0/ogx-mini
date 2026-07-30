#include <algorithm>
#include <array>
#include <cstdio>
#include <initializer_list>
#include <string_view>
#include <vector>

#include "Display/OLEDDisplay.h"
#include "I2CDriver/I2CDriver.h"

namespace
{
    constexpr uint8_t FONT_SPACE[] = {0x00, 0x00, 0x00, 0x00, 0x00};
    constexpr uint8_t FONT_DASH[]  = {0x08, 0x08, 0x08, 0x08, 0x08};
    constexpr uint8_t FONT_0[] = {0x3E, 0x51, 0x49, 0x45, 0x3E};
    constexpr uint8_t FONT_1[] = {0x00, 0x42, 0x7F, 0x40, 0x00};
    constexpr uint8_t FONT_2[] = {0x42, 0x61, 0x51, 0x49, 0x46};
    constexpr uint8_t FONT_3[] = {0x21, 0x41, 0x45, 0x4B, 0x31};
    constexpr uint8_t FONT_4[] = {0x18, 0x14, 0x12, 0x7F, 0x10};
    constexpr uint8_t FONT_5[] = {0x27, 0x45, 0x45, 0x45, 0x39};
    constexpr uint8_t FONT_6[] = {0x3C, 0x4A, 0x49, 0x49, 0x30};
    constexpr uint8_t FONT_7[] = {0x01, 0x71, 0x09, 0x05, 0x03};
    constexpr uint8_t FONT_8[] = {0x36, 0x49, 0x49, 0x49, 0x36};
    constexpr uint8_t FONT_9[] = {0x06, 0x49, 0x49, 0x29, 0x1E};
    constexpr uint8_t FONT_A[] = {0x7E, 0x11, 0x11, 0x11, 0x7E};
    constexpr uint8_t FONT_B[] = {0x7F, 0x49, 0x49, 0x49, 0x36};
    constexpr uint8_t FONT_C[] = {0x3E, 0x41, 0x41, 0x41, 0x22};
    constexpr uint8_t FONT_D[] = {0x7F, 0x41, 0x41, 0x22, 0x1C};
    constexpr uint8_t FONT_E[] = {0x7F, 0x49, 0x49, 0x49, 0x41};
    constexpr uint8_t FONT_F[] = {0x7F, 0x09, 0x09, 0x09, 0x01};
    constexpr uint8_t FONT_G[] = {0x3E, 0x41, 0x49, 0x49, 0x3A};
    constexpr uint8_t FONT_H[] = {0x7F, 0x08, 0x08, 0x08, 0x7F};
    constexpr uint8_t FONT_I[] = {0x00, 0x41, 0x7F, 0x41, 0x00};
    constexpr uint8_t FONT_J[] = {0x20, 0x40, 0x41, 0x3F, 0x01};
    constexpr uint8_t FONT_K[] = {0x7F, 0x08, 0x14, 0x22, 0x41};
    constexpr uint8_t FONT_L[] = {0x7F, 0x40, 0x40, 0x40, 0x40};
    constexpr uint8_t FONT_M[] = {0x7F, 0x02, 0x0C, 0x02, 0x7F};
    constexpr uint8_t FONT_N[] = {0x7F, 0x04, 0x08, 0x10, 0x7F};
    constexpr uint8_t FONT_O[] = {0x3E, 0x41, 0x41, 0x41, 0x3E};
    constexpr uint8_t FONT_P[] = {0x7F, 0x09, 0x09, 0x09, 0x06};
    constexpr uint8_t FONT_Q[] = {0x3E, 0x41, 0x51, 0x21, 0x5E};
    constexpr uint8_t FONT_R[] = {0x7F, 0x09, 0x19, 0x29, 0x46};
    constexpr uint8_t FONT_S[] = {0x46, 0x49, 0x49, 0x49, 0x31};
    constexpr uint8_t FONT_T[] = {0x01, 0x01, 0x7F, 0x01, 0x01};
    constexpr uint8_t FONT_U[] = {0x3F, 0x40, 0x40, 0x40, 0x3F};
    constexpr uint8_t FONT_V[] = {0x1F, 0x20, 0x40, 0x20, 0x1F};
    constexpr uint8_t FONT_W[] = {0x7F, 0x20, 0x18, 0x20, 0x7F};
    constexpr uint8_t FONT_X[] = {0x63, 0x14, 0x08, 0x14, 0x63};
    constexpr uint8_t FONT_Y[] = {0x03, 0x04, 0x78, 0x04, 0x03};
    constexpr uint8_t FONT_Z[] = {0x61, 0x51, 0x49, 0x45, 0x43};
}

void OLEDDisplay::initialize(I2CDriver& i2c_driver)
{
#if !CONFIG_ENABLE_OLED_DISPLAY
    (void)i2c_driver;
    return;
#else
    i2c_driver_ = &i2c_driver;

    if (initialized_)
    {
        return;
    }

    send_commands({
        0xAE, 0x20, 0x02, 0xB0, 0xC8, 0x00, 0x10, 0x40,
        0x81, 0x7F, 0xA1, 0xA6, 0xA8, 0x1F, 0xA4, 0xD3,
        0x00, 0xD5, 0x80, 0xD9, 0xF1, 0xDA, 0x02, 0xDB,
        0x20, 0x8D, 0x14, 0xAF
    });

    initialized_ = true;
    show_idle();
#endif
}

void OLEDDisplay::show_idle()
{
#if CONFIG_ENABLE_OLED_DISPLAY
    render_lines("OGX MINI", "BT READY", "PRESS PAIR", "TO SCAN");
#endif
}

void OLEDDisplay::show_pairing(uint32_t seconds_remaining)
{
#if CONFIG_ENABLE_OLED_DISPLAY
    char status_line[16];
    std::snprintf(status_line, sizeof(status_line), "TIME %lus", static_cast<unsigned long>(seconds_remaining));
    std::transform(status_line, status_line + std::char_traits<char>::length(status_line), status_line, [](unsigned char c)
    {
        return static_cast<char>((c >= 'a' && c <= 'z') ? (c - 'a' + 'A') : c);
    });

    render_lines("OGX MINI", "PAIR MODE", "WAIT PAD", status_line);
#endif
}

void OLEDDisplay::show_connected(uint8_t connected_count)
{
#if CONFIG_ENABLE_OLED_DISPLAY
    char count_line[16];
    std::snprintf(count_line, sizeof(count_line), "COUNT %u", connected_count);
    render_lines("OGX MINI", "PAD LINKED", "READY", count_line);
#endif
}

void OLEDDisplay::render_lines(
    std::string_view line1,
    std::string_view line2,
    std::string_view line3,
    std::string_view line4)
{
#if CONFIG_ENABLE_OLED_DISPLAY
    if (!initialized_ || i2c_driver_ == nullptr)
    {
        return;
    }

    std::array<uint8_t, BUFFER_SIZE> buffer{};
    draw_text(buffer, 0, line1);
    draw_text(buffer, 1, line2);
    draw_text(buffer, 2, line3);
    draw_text(buffer, 3, line4);

    for (uint8_t page = 0; page < PAGES; ++page)
    {
        send_commands({
            static_cast<uint8_t>(0xB0 + page),
            0x00,
            0x10
        });
        send_page(page, &buffer[page * WIDTH], WIDTH);
    }
#else
    (void)line1;
    (void)line2;
    (void)line3;
    (void)line4;
#endif
}

void OLEDDisplay::send_commands(std::initializer_list<uint8_t> commands)
{
#if CONFIG_ENABLE_OLED_DISPLAY
    if (i2c_driver_ == nullptr)
    {
        return;
    }

    std::vector<uint8_t> data;
    data.reserve(commands.size() + 1);
    data.push_back(0x00);
    data.insert(data.end(), commands.begin(), commands.end());
    i2c_driver_->write_bytes(I2C_ADDRESS, std::move(data));
#else
    (void)commands;
#endif
}

void OLEDDisplay::send_page(uint8_t page, const uint8_t* data, size_t len)
{
#if CONFIG_ENABLE_OLED_DISPLAY
    (void)page;

    if (i2c_driver_ == nullptr)
    {
        return;
    }

    std::vector<uint8_t> page_data;
    page_data.reserve(len + 1);
    page_data.push_back(0x40);
    page_data.insert(page_data.end(), data, data + len);
    i2c_driver_->write_bytes(I2C_ADDRESS, std::move(page_data));
#else
    (void)page;
    (void)data;
    (void)len;
#endif
}

const uint8_t* OLEDDisplay::font_for_char(char c)
{
    switch (c)
    {
        case '0': return FONT_0;
        case '1': return FONT_1;
        case '2': return FONT_2;
        case '3': return FONT_3;
        case '4': return FONT_4;
        case '5': return FONT_5;
        case '6': return FONT_6;
        case '7': return FONT_7;
        case '8': return FONT_8;
        case '9': return FONT_9;
        case 'A': return FONT_A;
        case 'B': return FONT_B;
        case 'C': return FONT_C;
        case 'D': return FONT_D;
        case 'E': return FONT_E;
        case 'F': return FONT_F;
        case 'G': return FONT_G;
        case 'H': return FONT_H;
        case 'I': return FONT_I;
        case 'J': return FONT_J;
        case 'K': return FONT_K;
        case 'L': return FONT_L;
        case 'M': return FONT_M;
        case 'N': return FONT_N;
        case 'O': return FONT_O;
        case 'P': return FONT_P;
        case 'Q': return FONT_Q;
        case 'R': return FONT_R;
        case 'S': return FONT_S;
        case 'T': return FONT_T;
        case 'U': return FONT_U;
        case 'V': return FONT_V;
        case 'W': return FONT_W;
        case 'X': return FONT_X;
        case 'Y': return FONT_Y;
        case 'Z': return FONT_Z;
        case '-': return FONT_DASH;
        case ' ':
        default:
            return FONT_SPACE;
    }
}

void OLEDDisplay::draw_text(std::array<uint8_t, BUFFER_SIZE>& buffer, uint8_t line, std::string_view text)
{
    if (line >= PAGES)
    {
        return;
    }

    constexpr uint8_t CHAR_WIDTH = 6;
    const size_t max_chars = WIDTH / CHAR_WIDTH;
    const size_t length = std::min(text.size(), max_chars);
    const uint8_t start_x = static_cast<uint8_t>((WIDTH - (length * CHAR_WIDTH)) / 2);
    const size_t offset = line * WIDTH + start_x;

    for (size_t i = 0; i < length; ++i)
    {
        char c = text[i];
        if (c >= 'a' && c <= 'z')
        {
            c = static_cast<char>(c - 'a' + 'A');
        }

        const uint8_t* glyph = font_for_char(c);
        for (uint8_t col = 0; col < 5; ++col)
        {
            buffer[offset + i * CHAR_WIDTH + col] = glyph[col];
        }
    }
}
