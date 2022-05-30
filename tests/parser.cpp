#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "calc.hpp"

double eval(std::string_view text)
{
    calc::Context ctx{};
    const auto expr = calc::parse(text);
    return expr->eval(ctx);
}

using namespace ::testing;

TEST(parser, empty_string_returns_null)
{
    auto e = calc::parse("");
    ASSERT_THAT(e, IsNull());
}

TEST(parser, all_whitespace_string_returns_null)
{
    auto e = calc::parse("    ");
    ASSERT_THAT(e, IsNull());
}

TEST(parser, on_invalid_parens_throws_exception)
{
    ASSERT_THROW(calc::parse("()("), std::runtime_error);
}

TEST(parser, binary_operators_plus)
{
    ASSERT_THAT(eval("2.1 + 3.2"), DoubleEq(5.3));
}

TEST(parser, binary_operators_minus)
{
    ASSERT_THAT(eval("2.1 - 3.2"), DoubleEq(-1.1));
}

TEST(parser, binary_operators_multiplies)
{
    ASSERT_THAT(eval("2.1 * 3.2"), DoubleEq(6.72));
}

TEST(parser, binary_operators_divides)
{
    ASSERT_THAT(eval("6.3 / 2.1"), DoubleEq(3.00));
}

TEST(parser, operator_precedence)
{
    ASSERT_THAT(eval("(2 + 3) * (3 - 1) - 1"), 9);
    ASSERT_THAT(eval("2 * 10 ^ 3"), 8000);
    ASSERT_THAT(eval("+(1 + 3)"), 4);
    ASSERT_THAT(eval("-(1 + 3)"), -4);
}