#pragma once

#include <cctype>
#include <functional>
#include <string_view>

namespace calc
{
inline std::string_view make_string_view(std::string_view::iterator b, std::string_view::iterator e)
{
    return { std::addressof(*b), std::string_view::size_type(e - b) };
}

inline std::string_view drop(std::string_view text, std::ptrdiff_t n)
{
    text.remove_prefix(std::min(n, static_cast<std::ptrdiff_t>(text.size())));
    return text;
}

inline std::string_view drop_last(std::string_view text, std::ptrdiff_t n)
{
    text.remove_suffix(std::min(n, static_cast<std::ptrdiff_t>(text.size())));
    return text;
}

inline std::string_view trim(std::string_view text, const std::function<bool(char)>& pred)
{
    while (!text.empty() && pred(text.front()))
    {
        text = drop(text, 1);
    }
    while (!text.empty() && pred(text.back()))
    {
        text = drop_last(text, 1);
    }
    return text;
}

inline std::string_view trim_whitespace(std::string_view text)
{
    return trim(text, [](char ch) { return std::isspace(ch); });
}

inline bool starts_with(std::string_view haystack, std::string_view needle)
{
    return haystack.substr(0, std::min(haystack.size(), needle.size())) == needle;
}

}  // namespace calc