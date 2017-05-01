/*

Tool for searching expressions involving a specified nr of operations and values.

Like the '10958' problem from Numberphile: https://www.youtube.com/watch?v=-ruC5A9EzzE

Author: Willem Hengeveld <itsme@xs4all.nl>
*/

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <cmath>
#include <experimental/optional>
#include "argparse.h"
#include <sys/time.h>
/*
clang++ -O3 -I ~/myprj/cpputils findexpr.cpp -std=c++1z

todo:
   support unary operators, like negation
*/


// class for taking usec resolution time measurements.
struct timer {
    struct timeval t0;
    timer()
    {
        gettimeofday(&t0, NULL);
    }
    uint64_t lap()
    {
        struct timeval t1;
        gettimeofday(&t1, NULL);

        uint64_t d = tdiff(t1, t0);
        t0 = t1;
        return d;
    }
    static uint64_t tdiff(struct timeval &lhs, struct timeval &rhs)
    {
        return uint64_t(lhs.tv_sec-rhs.tv_sec)*1000000ULL + uint64_t(lhs.tv_usec-rhs.tv_usec);
    }
};

// the base type we do our calculations in.
using T = double;

// represent an operation
struct Operation {

    // name  - the name of the operation
    // infix - when available: a symbols for infix operator notation ( a+b, instead of add(a,b) )
    // n     - the nr of arguments to the operation
    // prec  - the operator precedence
    // fn    - a lambda calculating this operation.
    Operation( std::string name, std::string infix, int n, int prec, std::function<T(std::vector<T> args)> fn)
        : name(name), infix(infix), n(n), precedence(prec), fn(fn)
    {
    }

    std::string name;
    std::string infix;
    int n;
    int precedence;
    std::function<T(std::vector<T> args)> fn;
};

// integer exponentiation
int intpow(int a, int b)
{
    int r = 1;
    while (b > 0) {
        if (b&1)
            r *= a;
        a *= a;
        b >>= 1;
    }
    return r;
}


// return 10^(trunc(log10(x))+1)
//
// calculates the smallest power of ten greater than x
double tenfactor(double x)
{
    double f = 1;
    int i=0;
    while (i<20 && x>=f) {
        f *= 10;
        i++;
    }
    return f;
}

// list of supported operations
std::vector<Operation> oplist{
    { "add", "+",   2, 1, [](std::vector<T> args){ return args[0]+args[1]; } },
    { "sub", "-",   2, 1, [](std::vector<T> args){ return args[0]-args[1]; } },
    { "mul", "*",   2, 2, [](std::vector<T> args){ return args[0]*args[1]; } },
    { "div", "/",   2, 3, [](std::vector<T> args){ return args[0]/args[1]; } },
    { "pow", "^",   2, 4, [](std::vector<T> args){ return pow(args[0],args[1]); } },
    { "cat", "||",  2, 5, [](std::vector<T> args){ return args[0]*tenfactor(args[1])+args[1]; } },
    { "neg", "-",   1, 2, [](std::vector<T> args){ return -args[0]; } },
};
int precedence(Operation*op)
{
    if (op==nullptr) // values have null.
        return 9;
    return op->precedence;
}

// expression tree base class
struct Node {
    using ptr = std::shared_ptr<Node>;
    virtual Operation *operation() const = 0;
    virtual T eval() const = 0;
    virtual void output(std::ostream& os) const = 0;

    friend std::ostream& operator<<(std::ostream& os, Node::ptr t)
    {
        t->output(os);
        return os;
    }
    friend std::ostream& operator<<(std::ostream& os, const Node &t)
    {
        t.output(os);
        return os;
    }

};

// represent a value node
struct Value : Node {
    T value;
    virtual Operation *operation() const { return nullptr; }
    virtual T eval() const { return value; }
    virtual void output(std::ostream& os) const { os << value; }

    static auto make()
    {
        return std::make_shared<Value>();
    }
};

// represent an expression node.
struct Expr : Node {
    Operation *op;
    std::vector<Node::ptr> args;
    Expr(Node::ptr l)
        : op(nullptr), args{l}
    {
    }
    Expr(Node::ptr l, Node::ptr r)
        : op(nullptr), args{l, r}
    {
    }
    virtual Operation *operation() const { return op; }
    virtual T eval() const
    {
        std::vector<T> results;
        for (auto v : args)
            results.push_back(v->eval());
        return op->fn(results);
    }
    virtual void output(std::ostream& os) const
    {
        if (!op) {
            // no operation specified: use placeholder 'op' / '#'
            if (args.size()==2) {
                // binary operator
                os << "(" << args[0] << '#' << args[1] << ")";
            }
            else if (args.size()==1) {
                // unary operator
                os << "(" << "-" << args[0] << ")";
            }
            else {
                // more args: represent as function call.
                os << "op(";
                bool first = true;
                for (auto arg : args)
                {
                    if (!first) os << ",";
                    os << arg;
                    first = false;
                }
                os << ")";
            }
        }
        else if (args.size()==2 && !op->infix.empty()) {
            // todo: num||num  -> num num
            // todo: num||(num||num)  -> num num num
            // todo: (num||num)||num  -> num num num
            //
            // (a+b) * c
            bool needbrackets0 = precedence(op) > precedence(args[0]->operation());
            if (needbrackets0) os << '(';
            os << args[0];
            if (needbrackets0) os << ')';

            os << op->infix;

            bool needbrackets1 = precedence(op) > precedence(args[1]->operation());
            if (needbrackets1) os << '(';
            os << args[1];
            if (needbrackets1) os << ')';
        }
        else if (args.size()==1 && !op->infix.empty()) {
            // render a unary operator
            bool needbrackets0 = precedence(op) > precedence(args[0]->operation());
            os << op->infix;
            if (needbrackets0) os << '(';
            os << args[0];
            if (needbrackets0) os << ')';
        }
        else {
            // render a >= 3-ary operator, or operator without infix notation.
            os << op->name << "(";
            bool first = true;
            for (auto arg : args)
            {
                if (!first) os << ",";
                os << arg;
                first = false;
            }
            os << ")";
        }
    }
    // a binary operator
    static auto make(Node::ptr l, Node::ptr r)
    {
        return std::make_shared<Expr>(l, r);
    }

    // a unary operator
    static auto make(Node::ptr l)
    {
        return std::make_shared<Expr>(l);
    }

};

// generate all possible binary tree shapes with n leaves.
void enumtrees(int nleaves, std::function<void(Node::ptr)> cb)
{
    if (nleaves<1)
        return;
    if (nleaves==1) {
        cb(Value::make());
        return;
    }

    for (int i=1 ; i<nleaves ; i++)
    {
        enumtrees(nleaves-i, [&](auto t) {
                enumtrees(i, [&](auto s) {
                        cb(Expr::make(t, s));
                        });
                });
    }
}

template<typename P>
struct generator {
    P cur;
    P end;
    generator(P first, P last)
        : cur(first), end(last)
    {
    }
    auto next()
    {
        if (cur==end)
            throw std::runtime_error("end of iteration");
        return *cur++;
    }
};

template<typename V>
auto iter(const V& v)
{
    return generator<typename V::const_iterator>(v.cbegin(), v.cend());
}

// sets leaf nodes in the tree `t` with values from the generator `g`
template<typename GEN>
void setvalues(Node::ptr t, GEN &g)
{
    auto v = std::dynamic_pointer_cast<Value>(t);
    if (v) {
        v->value = g.next();
        return;
    }
    auto e =  std::dynamic_pointer_cast<Expr>(t);
    if (e) {
        for (auto a : e->args)
            setvalues(a, g);
    }
}

// sets expression nodes in the tree `t` with binary operations from the generator `g`
template<typename GEN>
void setops(Node::ptr t, GEN &g)
{
    auto e =  std::dynamic_pointer_cast<Expr>(t);
    if (e) {
        if (e->args.size()==2)
            e->op = g.next();
        for (auto a : e->args)
            setops(a, g);
    }
}

// generate operations given the index number `i`.
// use i as a n-ary number, each digit choosing an operation
// from the `ops` list.
struct OpsGenerator {
    std::vector<Operation*> ops;
    int cur;
    OpsGenerator(std::vector<Operation*> ops, int i)
        : ops(ops), cur(i)
    {
    }
    Operation* next()
    {
        Operation *op = ops[cur % ops.size()];
        cur /= ops.size();

        return op;
    }
};

int main(int argc,char**argv)
{
    std::vector<int> nums = { 1,2,3,4,5,6,7,8,9 };
    int digit = -1;
    int count = -1;
    std::experimental::optional<int> target;
    for (auto& arg : ArgParser(argc, argv))
       switch (arg.option())
       {
           case 'r': std::reverse(nums.begin(), nums.end()); break;
           case 'd': digit = arg.getint(); break;
           case 'n': count = arg.getint(); break;
           case 't': target = arg.getint(); break;
           default:
                     std::cout << "Usage: findexpr [-r] [-d DIGIT] [-n N] -[t TARGET]\n";
                     std::cout << "     -r     : use descending ( reverse ) order of numbers\n";
                     std::cout << "     -d D, -n N : use N times the digit D, instead of 1..9\n";
                     std::cout << "     -t T   : report only when result is near target\n";

                     return 1;

       }
    if (count > 0 && digit>0) {
        nums.clear();
        nums.resize(count, digit);
    }

    std::vector<Operation*> binops;
    for (int i = 0 ; i<oplist.size() ; i++)
        if (oplist[i].n==2)
            binops.push_back(&oplist[i]);

    timer t;

    // enum all tree shapes, then for each tree assign all possible combinations of operations
    // and the values from 1 - 9.
    enumtrees(nums.size(), [&](auto expr) {
            std::cout << "=========" << t.lap() << " usec   " << expr << std::endl;
            for (int i = 0 ; i < intpow(binops.size(), nums.size()-1) ; i++) {
                auto iops = OpsGenerator(binops, i);
                auto inums = iter(nums);
                setvalues(expr, inums);
                setops(expr, iops);
                double result = expr->eval();
                if (!target || fabs(result-*target)<=0.11)
                    std::cout << result << '=' << expr << std::endl;
            }
        });
}
