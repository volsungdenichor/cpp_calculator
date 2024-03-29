#include <cmath>
#include <deque>
#include <iomanip>
#include <iostream>

#include "ansi.hpp"
#include "calc.hpp"
#include "string_utils.hpp"

std::string read_line(std::function<void(std::ostream& os)> prompt)
{
    prompt(std::cout);
    std::string line;
    std::getline(std::cin, line);
    return std::string{ calc::trim_whitespace(line) };
}

struct History
{
    struct Entry
    {
        std::string expr;
        double result;
    };

    void push(std::string expr, double result)
    {
        if (entries.size() == max_size)
        {
            entries.pop_back();
        }
        entries.push_front({ std::move(expr), result });
    }

    std::size_t max_size;
    std::deque<Entry> entries;
};

int main(int argc, char* argv[])
{
    using namespace ansi;

    auto history = History{ 10 };
    calc::Context ctx{
        { { "pi", std::asin(1.0) * 2.0 } }
    };

    while (true)
    {
        const auto line = read_line([](std::ostream& os) { os << fg(color::green) << "> "; });
        if (line == "quit")
        {
            break;
        }
        else if (line == "vars")
        {
            for (const auto& [n, v] : ctx.vars)
            {
                std::cout << "  " << n << " = " << v << std::endl;
            }
        }
        else if (line == "history")
        {
            int i = 0;
            for (const auto& entry : history.entries)
            {
                std::cout << fg(color::dark_gray) << std::setw(2) << i++ << ". " << fg(color::dark_green) << entry.expr << fg(color::dark_blue) << " = " << entry.result << ansi::reset << std::endl;
            }
        }
        else
        {
            try
            {
                if (const auto expr = calc::parse(line))
                {
                    const auto res = expr->eval(ctx);
                    ctx.vars.insert_or_assign("ans", res);
                    history.push(line, res);
                    std::cout << fg(color::yellow) << "ans = " << res << reset << '\n';
                }
                else
                {
                    std::cout << "cannot parse expression" << '\n';
                }
            }
            catch (const std::exception& ex)
            {
                std::cout << "exception: " << ex.what() << '\n';
            }
        }
    }
    return 0;
}
