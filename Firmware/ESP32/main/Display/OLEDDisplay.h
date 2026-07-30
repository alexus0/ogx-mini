#ifndef _OLED_DISPLAY_H_
#define _OLED_DISPLAY_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string_view>

#include "sdkconfig.h"

class I2CDriver;

class OLEDDisplay
{
public:
    void initialize(I2CDriver& i2c_driver);
    void show_idle();
    void show_pairing(uint32_t seconds_remaining);
    void show_connected(uint8_t connected_count);

private:
    static constexpr uint8_t WIDTH = 128;
    static constexpr uint8_t HEIGHT = 32;
    static constexpr uint8_t PAGES = HEIGHT / 8;
    static constexpr uint8_t I2C_ADDRESS = CONFIG_OLED_I2C_ADDRESS;
    static constexpr size_t BUFFER_SIZE = WIDTH * PAGES;

    I2CDriver* i2c_driver_ = nullptr;
    bool initialized_ = false;

    void render_lines(
        std::string_view line1,
        std::string_view line2 = {},
        std::string_view line3 = {},
        std::string_view line4 = {}
    );
    void send_commands(std::initializer_list<uint8_t> commands);
    void send_page(uint8_t page, const uint8_t* data, size_t len);
    static const uint8_t* font_for_char(char c);
    static void draw_text(std::array<uint8_t, BUFFER_SIZE>& buffer, uint8_t line, std::string_view text);
};

#endif // _OLED_DISPLAY_H_
