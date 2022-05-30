#pragma once

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string_view>

namespace calc
{
struct Context
{
    std::map<std::string, double> vars;
};

using UnaryFunc = std::function<double(double)>;
using BinaryFunc = std::function<double(double, double)>;
using Func = std::function<double(const std::vector<double>&)>;

struct BinaryOpInfo
{
    std::string symbol;
    int precedence;
    bool right_associative;
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
    Func func;
};

struct Expr
{
    virtual ~Expr() = default;

    virtual double eval(Context& ctx) const = 0;
    virtual void print(std::ostream& os, int level) const = 0;
};

struct Parser
{
public:
    Parser();

    void register_function(std::string name, Func func);

    std::unique_ptr<Expr> operator()(std::string_view text) const;

    std::unique_ptr<Expr> parse_expr(std::string_view text) const;

private:
    struct BinaryOpResult
    {
        std::string_view::iterator iter;
        const BinaryOpInfo* op_info;
    };

    std::unique_ptr<Expr> parse_variable(std::string_view text) const;

    std::unique_ptr<Expr> parse_number(std::string_view text) const;

    std::unique_ptr<Expr> parse_unary(std::string_view text) const;

    std::unique_ptr<Expr> parse_binary(std::string_view text) const;

    std::unique_ptr<Expr> parse_function(std::string_view text) const;

    std::optional<BinaryOpResult> find_binary_oper(std::string_view text) const;

    std::vector<UnaryOpInfo> unary_op_info_list;
    std::vector<BinaryOpInfo> binary_op_info_list;
    std::vector<FuncInfo> function_info_list;
};

static const inline auto parse = Parser{};

}  // namespace calc
