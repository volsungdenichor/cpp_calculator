#include <iostream>

#include "calc.hpp"

int main(int argc, char* argv[])
{
    if (const auto e = calc::parse("min(2, 4, 10 - 2 * 4.5, -(7 - 10))"))
    {
        calc::Context ctx{};
        e->print(std::cout, 0);
        std::cout << e->eval(ctx) << std::endl;
    }
}
