#include "calc.hpp"
#include <algorithm>
#include <cmath>

namespace calc
{
std::string_view make_string_view(std::string_view::iterator b, std::string_view::iterator e)
{
    return { std::addressof(*b), std::string_view::size_type(e - b) };
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
    catch (const std::exception&)
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

std::string_view drop(std::string_view text, std::ptrdiff_t n)
{
    text.remove_prefix(std::min(n, static_cast<std::ptrdiff_t>(text.size())));
    return text;
}

std::string_view drop_last(std::string_view text, std::ptrdiff_t n)
{
    text.remove_suffix(std::min(n, static_cast<std::ptrdiff_t>(text.size())));
    return text;
}

std::string_view trim_whitespace(std::string_view text)
{
    while (!text.empty() && text.front() == ' ')
    {
        text = drop(text, 1);
    }
    while (!text.empty() && text.back() == ' ')
    {
        text = drop_last(text, 1);
    }
    return text;
}

bool starts_with(std::string_view haystack, std::string_view needle)
{
    return haystack.substr(0, std::min(haystack.size(), needle.size())) == needle;
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

struct ValueExpr : public Expr
{
    double v;

    ValueExpr(double v)
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

struct VariableExpr : public Expr
{
    std::string name;

    VariableExpr(std::string name)
        : name{ std::move(name) }
    {
    }

    double eval(Context& ctx) const override
    {
        auto it = ctx.vars.find(name);
        if (it == ctx.vars.end())
        {
            throw std::runtime_error{ "undefined variable '" + name + "'" };
        }
        return it->second;
    }

    void print(std::ostream& os, int level) const override
    {
        os << indent(level) << name << std::endl;
    }
};

struct UnaryOpExpr : public Expr
{
    std::reference_wrapper<const UnaryOpInfo> info;
    std::unique_ptr<Expr> sub;

    UnaryOpExpr(
        std::reference_wrapper<const UnaryOpInfo> info, std::unique_ptr<Expr> sub)
        : info{ info }
        , sub{ std::move(sub) }
    {
    }

    double eval(Context& ctx) const override
    {
        return info.get().func(sub->eval(ctx));
    }

    void print(std::ostream& os, int level) const override
    {
        os << indent(level) << info.get().symbol << std::endl;
        sub->print(os, level + 1);
    }
};

struct BinaryOpExpr : public Expr
{
    std::reference_wrapper<const BinaryOpInfo> info;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;

    BinaryOpExpr(std::reference_wrapper<const BinaryOpInfo> info, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs)
        : info{ info }
        , lhs{ std::move(lhs) }
        , rhs{ std::move(rhs) }
    {
    }

    double eval(Context& ctx) const override
    {
        return info.get().func(lhs->eval(ctx), rhs->eval(ctx));
    }

    void print(std::ostream& os, int level) const override
    {
        os << indent(level) << info.get().symbol << std::endl;
        lhs->print(os, level + 1);
        rhs->print(os, level + 1);
    }
};

struct FuncExpr : public Expr
{
    std::reference_wrapper<const FuncInfo> info;
    std::vector<std::unique_ptr<Expr>> subs;

    FuncExpr(std::reference_wrapper<const FuncInfo> info, std::vector<std::unique_ptr<Expr>> subs)
        : info{ info }
        , subs{ std::move(subs) }
    {
    }

    double eval(Context& ctx) const override
    {
        std::vector<double> args(subs.size());
        std::transform(subs.begin(), subs.end(), args.begin(), [&](const auto& expr_ptr) { return expr_ptr->eval(ctx); });
        return info.get().func(args);
    }

    void print(std::ostream& os, int level) const override
    {
        os << indent(level) << info.get().name << std::endl;
        for (std::size_t i = 0; i < subs.size(); ++i)
        {
            subs[i]->print(os, level + 1);
        }
    }
};

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

Parser::Parser()
{
    binary_op_info_list.push_back(BinaryOpInfo{ "+", 2, false, std::plus<>{} });
    binary_op_info_list.push_back(BinaryOpInfo{ "-", 2, false, std::minus<>{} });
    binary_op_info_list.push_back(BinaryOpInfo{ "*", 4, false, std::multiplies<>{} });
    binary_op_info_list.push_back(BinaryOpInfo{ "/", 4, false, std::divides<>{} });
    binary_op_info_list.push_back(BinaryOpInfo{ "^", 3, true, binary_pow });

    binary_op_info_list.push_back(BinaryOpInfo{ "==", 1, false, std::equal_to<>{} });
    binary_op_info_list.push_back(BinaryOpInfo{ "!=", 1, false, std::not_equal_to<>{} });
    binary_op_info_list.push_back(BinaryOpInfo{ "<", 1, false, std::less<>{} });
    binary_op_info_list.push_back(BinaryOpInfo{ "<=", 1, false, std::less_equal<>{} });
    binary_op_info_list.push_back(BinaryOpInfo{ ">", 1, false, std::greater<>{} });
    binary_op_info_list.push_back(BinaryOpInfo{ ">=", 1, false, std::greater_equal<>{} });

    unary_op_info_list.push_back(UnaryOpInfo{ "+", unary_pos });
    unary_op_info_list.push_back(UnaryOpInfo{ "-", std::negate<>{} });

    register_function("sin", func_sin);
    register_function("cos", func_cos);
    register_function("max", func_max);
    register_function("min", func_min);
    register_function("sqrt", func_sqrt);
}

void Parser::register_function(std::string name, Func func)
{
    function_info_list.push_back(FuncInfo{ std::move(name), std::move(func) });
}

std::unique_ptr<Expr> Parser::operator()(std::string_view text) const
{
    return parse_expr(text);
}

std::unique_ptr<Expr> Parser::parse_expr(std::string_view text) const
{
    if (!valid_parens(text))
    {
        throw std::runtime_error{ "Invalid parens" };
    }

    text = simplify_parens(text);

    static const auto methods = std::array{ &Parser::parse_number,
                                            &Parser::parse_binary,
                                            &Parser::parse_unary,
                                            &Parser::parse_function,
                                            &Parser::parse_variable };

    for (const auto& method : methods)
    {
        if (auto res = ((*this).*method)(text))
        {
            return res;
        }
    }

    return nullptr;
}

std::unique_ptr<Expr> Parser::parse_variable(std::string_view text) const
{
    if (!text.empty() && std::all_of(text.begin(), text.end(), [](char ch) { return std::isalpha(ch) || ch == '_'; }))
    {
        return std::make_unique<VariableExpr>(std::string{ text });
    }
    return nullptr;
}

std::unique_ptr<Expr> Parser::parse_number(std::string_view text) const
{
    if (auto res = parse_double(text))
    {
        return std::make_unique<ValueExpr>(*res);
    }
    return nullptr;
}

std::unique_ptr<Expr> Parser::parse_unary(std::string_view text) const
{
    for (const auto& op_info : unary_op_info_list)
    {
        if (starts_with(text, op_info.symbol))
        {
            if (auto sub = parse_expr(make_string_view(text.begin() + op_info.symbol.size(), text.end())))
            {
                return std::make_unique<UnaryOpExpr>(std::cref(op_info), std::move(sub));
            }
        }
    }
    return nullptr;
}

std::unique_ptr<Expr> Parser::parse_binary(std::string_view text) const
{
    const auto oper_result = find_binary_oper(text);
    if (!oper_result)
    {
        return nullptr;
    }

    const auto [iter, op_info] = *oper_result;

    auto lhs = parse_expr(make_string_view(text.begin(), iter));
    auto rhs = parse_expr(make_string_view(iter + op_info->symbol.size(), text.end()));
    if (lhs && rhs)
    {
        return std::make_unique<BinaryOpExpr>(std::cref(*op_info), std::move(lhs), std::move(rhs));
    }
    return nullptr;
}

std::unique_ptr<Expr> Parser::parse_function(std::string_view text) const
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
        std::vector<std::unique_ptr<Expr>> subs;
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
    return std::make_unique<FuncExpr>(std::cref(*info), std::move(subs));
}

std::optional<Parser::BinaryOpResult> Parser::find_binary_oper(std::string_view text) const
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
                    if ((op_info.precedence <= min_precedence && !op_info.right_associative) || (op_info.precedence < min_precedence && op_info.right_associative))
                    {
                        min_precedence = op_info.precedence;
                        res = BinaryOpResult{ it, &op_info };
                    }
                }
            }
        }
    }
    return res;
}

}  // namespace calc
