#include "calc.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <optional>

#include "string_utils.hpp"

namespace calc
{
void assert(bool value)
{
    if (!value)
        throw std::logic_error{ "assertion failed" };
}

std::optional<double> parse_double(std::string_view text)
{
    try
    {
        std::size_t size{};
        const double res = std::stod(std::string{ text }, &size);
        if (size != text.size())
        {
            return std::nullopt;
        }
        return res;
    }
    catch (const std::invalid_argument&)
    {
        return std::nullopt;
    }
}

bool valid_parens(std::string_view text)
{
    int counter = 0;
    for (char ch : text)
    {
        if (ch == '(')
        {
            ++counter;
        }
        else if (ch == ')')
        {
            --counter;
            if (counter < 0)
            {
                return false;
            }
        }
    }
    return counter == 0;
}

std::string_view simplify_parens(std::string_view text)
{
    while (true)
    {
        text = trim_whitespace(text);
        if (!text.empty() && text.front() == '(' && text.back() == ')')
        {
            auto temp = trim_whitespace(drop_last(drop(text, 1), 1));
            if (valid_parens(temp))
            {
                text = temp;
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }
    return text;
}

std::string indent(int level)
{
    return std::string(level * 2, ' ');
}

using UnaryFunc = std::function<double(double)>;
using BinaryFunc = std::function<double(double, double)>;

struct Precedence
{
    int value;
    bool right_associative;

    friend bool operator<(const Precedence& precedence, int value)
    {
        return (precedence.value <= value && !precedence.right_associative) || (precedence.value < value && precedence.right_associative);
    }
};

Precedence left_associative(int v)
{
    return { v, false };
}

Precedence right_associative(int v)
{
    return { v, true };
}

struct BinaryOpInfo
{
    std::string symbol;
    Precedence precedence;
    BinaryFunc func;
};

struct UnaryOpInfo
{
    std::string symbol;
    UnaryFunc func;
};

struct FuncInfo
{
    std::string name;
    Function func;
};

bool is_assignment(const BinaryOpInfo& op_info)
{
    return !op_info.func;
}

namespace expressions
{
struct Value : public Expr
{
    double v;

    Value(double v)
        : v{ v }
    {
    }

    double eval(Context& ctx) const override
    {
        return v;
    }

    void print(std::ostream& os, int level) const override
    {
        os << indent(level) << v << std::endl;
    }
};

struct Variable : public Expr
{
    std::string name;

    Variable(std::string name)
        : name{ std::move(name) }
    {
    }

    double eval(Context& ctx) const override
    {
        if (auto it = ctx.vars.find(name); it != ctx.vars.end())
        {
            return it->second;
        }
        throw std::runtime_error{ "undefined variable '" + name + "'" };
    }

    void print(std::ostream& os, int level) const override
    {
        os << indent(level) << name << std::endl;
    }
};

struct UnaryOp : public Expr
{
    const UnaryOpInfo& info;
    ExprPtr sub;

    UnaryOp(const UnaryOpInfo& info, ExprPtr sub)
        : info{ info }
        , sub{ std::move(sub) }
    {
    }

    double eval(Context& ctx) const override
    {
        return info.func(sub->eval(ctx));
    }

    void print(std::ostream& os, int level) const override
    {
        os << indent(level) << info.symbol << std::endl;
        sub->print(os, level + 1);
    }
};

struct BinaryOp : public Expr
{
    const BinaryOpInfo& info;
    ExprPtr lhs;
    ExprPtr rhs;

    BinaryOp(const BinaryOpInfo& info, ExprPtr lhs, ExprPtr rhs)
        : info{ info }
        , lhs{ std::move(lhs) }
        , rhs{ std::move(rhs) }
    {
    }

    double eval(Context& ctx) const override
    {
        return info.func(lhs->eval(ctx), rhs->eval(ctx));
    }

    void print(std::ostream& os, int level) const override
    {
        os << indent(level) << info.symbol << std::endl;
        lhs->print(os, level + 1);
        rhs->print(os, level + 1);
    }
};

struct Func : public Expr
{
    const FuncInfo& info;
    std::vector<ExprPtr> subs;

    Func(const FuncInfo& info, std::vector<ExprPtr> subs)
        : info{ info }
        , subs{ std::move(subs) }
    {
    }

    double eval(Context& ctx) const override
    {
        std::vector<double> args(subs.size());
        std::transform(subs.begin(), subs.end(), args.begin(), [&](const auto& expr_ptr) { return expr_ptr->eval(ctx); });
        return info.func(args);
    }

    void print(std::ostream& os, int level) const override
    {
        os << indent(level) << info.name << std::endl;
        for (std::size_t i = 0; i < subs.size(); ++i)
        {
            subs[i]->print(os, level + 1);
        }
    }
};

struct Assignment : public Expr
{
    std::string name;
    ExprPtr expr;

    Assignment(std::string name, ExprPtr expr)
        : name{ std::move(name) }
        , expr{ std::move(expr) }
    {
    }

    double eval(Context& ctx) const override
    {
        ctx.vars.insert_or_assign(name, expr->eval(ctx));
        return ctx.vars.at(name);
    }

    void print(std::ostream& os, int level) const override
    {
        os << indent(level) << name << std::endl;
        expr->print(os, level + 1);
    }
};

}  // namespace expressions

static double unary_pos(double x)
{
    return x;
}

static double unary_neg(double x)
{
    return -x;
}

static double binary_pow(double x, double y)
{
    return std::pow(x, y);
}

static double func_sum(const std::vector<double>& args)
{
    return std::accumulate(args.begin(), args.end(), 0.0);
}

static double func_sin(const std::vector<double>& args)
{
    return std::sin(args.at(0));
}

static double func_cos(const std::vector<double>& args)
{
    return std::cos(args.at(0));
}

static double func_max(const std::vector<double>& args)
{
    return *std::max_element(args.begin(), args.end());
}

static double func_min(const std::vector<double>& args)
{
    return *std::min_element(args.begin(), args.end());
}

static double func_sqrt(const std::vector<double>& args)
{
    return std::sqrt(args.at(0));
}

struct BinaryOpResult
{
    std::string_view::iterator it;
    const BinaryOpInfo* op_info;
};

bool is_valid_variable_name(std::string_view text)
{
    return !text.empty() && std::all_of(text.begin(), text.end(), [](char ch) { return std::isalpha(ch) || ch == '_'; });
}

struct Parser::Impl
{
    Impl()
    {
        binary_op_info_list.push_back(BinaryOpInfo{ "==", left_associative(10), std::equal_to<>{} });
        binary_op_info_list.push_back(BinaryOpInfo{ "!=", left_associative(10), std::not_equal_to<>{} });
        binary_op_info_list.push_back(BinaryOpInfo{ "<", left_associative(10), std::less<>{} });
        binary_op_info_list.push_back(BinaryOpInfo{ "<=", left_associative(10), std::less_equal<>{} });
        binary_op_info_list.push_back(BinaryOpInfo{ ">", left_associative(10), std::greater<>{} });
        binary_op_info_list.push_back(BinaryOpInfo{ ">=", left_associative(10), std::greater_equal<>{} });

        binary_op_info_list.push_back(BinaryOpInfo{ "+", left_associative(20), std::plus<>{} });
        binary_op_info_list.push_back(BinaryOpInfo{ "-", left_associative(20), std::minus<>{} });
        binary_op_info_list.push_back(BinaryOpInfo{ "*", left_associative(40), std::multiplies<>{} });
        binary_op_info_list.push_back(BinaryOpInfo{ "/", left_associative(40), std::divides<>{} });
        binary_op_info_list.push_back(BinaryOpInfo{ "^", right_associative(30), binary_pow });

        binary_op_info_list.push_back(BinaryOpInfo{ "=", left_associative(5), nullptr });

        unary_op_info_list.push_back(UnaryOpInfo{ "+", unary_pos });
        unary_op_info_list.push_back(UnaryOpInfo{ "-", std::negate<>{} });
    }

    void register_function(std::string name, Function func)
    {
        function_info_list.push_back(FuncInfo{ std::move(name), std::move(func) });
    }

    ExprPtr parse_expr(std::string_view text) const
    {
        if (!valid_parens(text))
        {
            throw std::runtime_error{ "Invalid parens" };
        }

        text = simplify_parens(text);

        static const auto methods = std::array{ &Impl::parse_number,
                                                &Impl::parse_binary_or_assignment,
                                                &Impl::parse_unary,
                                                &Impl::parse_function,
                                                &Impl::parse_variable };

        for (const auto& method : methods)
        {
            if (auto res = ((*this).*method)(text))
            {
                return res;
            }
        }

        return nullptr;
    }

    ExprPtr parse_number(std::string_view text) const
    {
        if (auto res = parse_double(text))
        {
            return std::make_unique<expressions::Value>(*res);
        }
        return nullptr;
    }

    ExprPtr parse_variable(std::string_view text) const
    {
        if (is_valid_variable_name(text))
        {
            return std::make_unique<expressions::Variable>(std::string{ text });
        }
        return nullptr;
    }

    ExprPtr parse_unary(std::string_view text) const
    {
        for (const auto& op_info : unary_op_info_list)
        {
            if (starts_with(text, op_info.symbol))
            {
                if (auto sub = parse_expr(make_string_view(text.begin() + op_info.symbol.size(), text.end())))
                {
                    return std::make_unique<expressions::UnaryOp>(op_info, std::move(sub));
                }
            }
        }
        return nullptr;
    }

    ExprPtr parse_binary_or_assignment(std::string_view text) const
    {
        const auto oper_result = find_binary_oper(text);
        if (!oper_result)
        {
            return nullptr;
        }

        const auto [it, op_info] = *oper_result;

        auto rhs = parse_expr(make_string_view(it + op_info->symbol.size(), text.end()));
        if (!rhs)
        {
            return nullptr;
        }

        const auto lhs_str = trim_whitespace(make_string_view(text.begin(), it));

        if (is_valid_variable_name(lhs_str) && is_assignment(*op_info))
        {
            return std::make_unique<expressions::Assignment>(std::string{ lhs_str }, std::move(rhs));
        }
        else if (auto lhs = parse_expr(lhs_str))
        {
            return std::make_unique<expressions::BinaryOp>(*op_info, std::move(lhs), std::move(rhs));
        }
        return nullptr;
    }

    ExprPtr parse_function(std::string_view text) const
    {
        auto it = std::find(text.begin(), text.end(), '(');
        if (it == text.end() || text.back() != ')')
        {
            return nullptr;
        }
        const auto name = trim_whitespace(make_string_view(text.begin(), it));
        const auto info = std::invoke([&]() -> const FuncInfo* {
            for (const auto& func_info : function_info_list)
            {
                if (func_info.name == name)
                {
                    return &func_info;
                }
            }
            return nullptr;
        });
        if (!info)
        {
            return nullptr;
        }
        auto subs = std::invoke([&]() {
            std::vector<ExprPtr> subs;
            text = simplify_parens(make_string_view(it, text.end()));
            while (!text.empty())
            {
                auto next = std::invoke([&]() {
                    int paren_counter = 0;
                    for (auto it = text.begin(); it != text.end(); ++it)
                    {
                        if (*it == '(')
                        {
                            ++paren_counter;
                        }
                        if (*it == ')')
                        {
                            --paren_counter;
                        }

                        if (*it == ',' && paren_counter == 0)
                        {
                            return it;
                        }
                    }
                    return text.end();
                });
                subs.push_back(parse_expr(make_string_view(text.begin(), next)));
                text = simplify_parens(drop(make_string_view(next, text.end()), 1));
            }
            return subs;
        });
        return std::make_unique<expressions::Func>(*info, std::move(subs));
    }

    std::optional<BinaryOpResult> find_binary_oper(std::string_view text) const
    {
        auto res = std::optional<BinaryOpResult>{};
        int min_precedence = std::numeric_limits<int>::max();
        int paren_counter = 0;
        for (auto it = text.begin(); it != text.end(); ++it)
        {
            if (*it == '(')
            {
                ++paren_counter;
            }
            if (*it == ')')
            {
                --paren_counter;
            }
            if (paren_counter == 0 && it != text.begin())
            {
                const auto sub = make_string_view(it, text.end());
                for (const auto& op_info : binary_op_info_list)
                {
                    if (starts_with(sub, op_info.symbol))
                    {
                        if (op_info.precedence < min_precedence)
                        {
                            min_precedence = op_info.precedence.value;
                            res = BinaryOpResult{ it, &op_info };
                        }
                    }
                }
            }
        }
        return res;
    }

    std::vector<UnaryOpInfo> unary_op_info_list;
    std::vector<BinaryOpInfo> binary_op_info_list;
    std::vector<FuncInfo> function_info_list;
};

Parser::Parser()
    : impl{ std::make_unique<Impl>() }
{
    register_function("sum", func_sum);
    register_function("sin", func_sin);
    register_function("cos", func_cos);
    register_function("max", func_max);
    register_function("min", func_min);
    register_function("sqrt", func_sqrt);
}

Parser::~Parser() = default;

void Parser::register_function(std::string name, Function func)
{
    impl->register_function(name, func);
}

ExprPtr Parser::operator()(std::string_view text) const
{
    return impl->parse_expr(text);
}

}  // namespace calc
