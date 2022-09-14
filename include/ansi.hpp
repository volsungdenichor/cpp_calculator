#pragma once

#include <ostream>

namespace ansi
{
struct ansi_t
{
    const int value;

    friend std::ostream& operator<<(std::ostream& os, const ansi_t& item)
    {
        return os << "\33[" << item.value << "m";
    }
};

enum class color
{
    black = 0,
    dark_red = 1,
    dark_green = 2,
    dark_yellow = 3,
    dark_blue = 4,
    dark_magenta = 5,
    dark_cyan = 6,
    gray = 7,
    dark_gray = 60,
    red = 61,
    green = 62,
    yellow = 63,
    blue = 64,
    magenta = 65,
    cyan = 66,
    white = 67,
};

constexpr inline ansi_t fg(color c)
{
    return { 30 + (int)c };
}

constexpr inline ansi_t bg(color c)
{
    return { 40 + (int)c };
}

static constexpr inline auto reset = ansi_t{ 0 };

}  // namespace ansi
