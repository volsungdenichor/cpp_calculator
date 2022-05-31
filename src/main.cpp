#include <cmath>
#include <deque>
#include <iostream>

#include "ansi.hpp"
#include "calc.hpp"

std::string read_line(std::istream& is, std::function<void(std::ostream& os)> prompt)
{
    prompt(std::cout);
    std::string line;
    std::getline(is, line);
    return line;
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
        { { "pi", std::asin(1) * 2 } }
    };

    while (true)
    {
        const auto line = read_line(std::cin, [](std::ostream& os) { os << fg(color::green) << "> "; });
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
            for (const auto& [expr, result] : history.entries)
            {
                std::cout << "  " << expr << " = " << result << std::endl;
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
                    std::cout << fg(color::yellow) << "ans = " << res << reset << std::endl;
                }
            }
            catch (const std::exception& ex)
            {
                std::cerr << "exception: " << ex.what() << '\n';
            }
        }
    }
    return 0;
}
