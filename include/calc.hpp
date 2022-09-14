#pragma once

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string_view>

namespace calc
{
struct Context
{
    std::map<std::string, double> vars;
};

using Function = std::function<double(const std::vector<double>&)>;

struct Expr
{
    virtual ~Expr() = default;

    virtual double eval(Context& ctx) const = 0;
    virtual void print(std::ostream& os, int level) const = 0;
};

using ExprPtr = std::unique_ptr<Expr>;

struct Parser
{
public:
    Parser();
    ~Parser();

    void register_function(std::string name, Function func);

    ExprPtr operator()(std::string_view text) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

static const inline auto parse = Parser{};

}  // namespace calc
