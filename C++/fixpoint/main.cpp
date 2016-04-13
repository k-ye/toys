//
//  main.cpp
//  Fix-point Solver
//
//  Created by Ye Kuang on 4/12/16.
//  Copyright © 2016 Ye Kuang. All rights reserved.
//

#include <iostream>
#include <functional>
#include <string>

// HO: High order function
// YC: Y Combinator. Actually it's a fix point solver. Not sure if real YC could
// be done in C++ .
template <typename HO>
struct YTrait {
    typedef typename HO::arg_type arg_type;
    typedef typename HO::ret_type ret_type;
    typedef std::function<ret_type(arg_type)> fun_type;
};

template <typename HO>
typename YTrait<HO>::fun_type YC(HO ho) {
    using arg_type = typename YTrait<HO>::arg_type;

    auto fixpt = [=](arg_type x) {
        return YC(ho)(x);
    };

    return ho(fixpt);
}

struct FactorialHO {
    typedef unsigned arg_type;
    typedef unsigned ret_type;

private:
    typedef YTrait<FactorialHO>::fun_type ho_type;

public:

    ho_type operator()(ho_type f) const {
        auto func = [=](arg_type n) {
            if (n == 0) {
                return 1U;
            } else {
                return n * f(n - 1);
            }
        };

        return func;
    }
};

int main() {
    int n = 6;
    auto factorial = YC(FactorialHO());
    std::cout << factorial(n) << std::endl;

    return 0;
}
// template <typename FUNC>
