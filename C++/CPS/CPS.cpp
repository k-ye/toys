#include <unordered_map>
#include <iostream>

typedef std::unordered_map<char, unsigned> StoreType;

template <unsigned n>
class ConstExpr { };

template <char c>
class VarExpr { };

template <typename Left, typename Right, typename Op>
class OpExpr { };

class AddOp
{
public:
	static unsigned compute(unsigned n, unsigned m) { return n + m; }
};

class MinusOp
{
public:
	static unsigned compute(unsigned n, unsigned m) { return n - m; }
};

class MultOp
{
public:
	static unsigned compute(unsigned n, unsigned m) { return n * m; }
};

class DivOp
{
public:
	static unsigned compute(unsigned n, unsigned m) { return n / m; }
};

template <typename Left, typename Right>
using AddExpr = OpExpr<Left, Right, AddOp>;

template <typename Left, typename Right>
using MinusExpr = OpExpr<Left, Right, MinusOp>;

template <typename Left, typename Right>
using MultExpr = OpExpr<Left, Right, MultOp>;

template <typename Left, typename Right>
using DivExpr = OpExpr<Left, Right, DivOp>;

template <char c, typename Expr>
class LambdaExpr { };

template <typename Expr1, typename Expr2>
class ApplyExpr { };

template <typename Expr>
class CpsImpl { };

template <unsigned n>
class CpsImpl< ConstExpr<n> >
{
public:
	template <typename Continuation>
	static unsigned apply(Continuation cont, StoreType&)
	{
		return cont(n);
	}
};

template <char c>
class CpsImpl< VarExpr<c> >
{
public:
	template <typename Continuation>
	static unsigned apply(Continuation cont, StoreType& store)
	{
		return cont(store.at(c));
	}
};

template <typename Left, typename Right, typename Op>
class CpsImpl< OpExpr<Left, Right, Op> >
{
public:
	template <typename Continuation>
	static unsigned apply(Continuation cont, StoreType& store)
	{
		return CpsImpl<Left>::apply(
			[&](unsigned n) -> unsigned
			{
				return CpsImpl<Right>::apply(
					[&](unsigned m) -> unsigned
					{
						return cont(Op::compute(n, m));
					}, store
				);
			}, store
		);
	}
};

template <char c, typename Expr>
class CpsImpl< LambdaExpr<c, Expr> >
{
public:
	template <typename Continuation>
	static unsigned apply(Continuation cont, StoreType& store)
	{
		return cont(
			[&](unsigned v, auto cont_)
			{
				store.insert(std::make_pair(c, v));
				return CpsImpl<Expr>::apply(cont_, store);
			}
		);
	}
};

template <typename Expr1, typename Expr2>
class CpsImpl< ApplyExpr<Expr1, Expr2> >
{
public:
	template <typename Continuation>
	static unsigned apply(Continuation cont_, StoreType& store)
	{
		return CpsImpl<Expr1>::apply(
			[&](auto expr1Cps)
			{
				return CpsImpl<Expr2>::apply(
					[&](unsigned v)
					{
						return expr1Cps(v, cont_);
					}, store
				);
			}, store
		);
	}
};

template <typename Expr>
unsigned cps()
{
	StoreType store;
	return CpsImpl<Expr>::apply([](unsigned v) { return v; }, store);
}


int main()
{
	std::cout 
	<< 
		// ((\x. (\y. x + y)) 5) 6
		cps< ApplyExpr< ApplyExpr< LambdaExpr< 'x', LambdaExpr< 'y', AddExpr< VarExpr<'x'>, VarExpr<'y'>> > >, ConstExpr<5> >, ConstExpr<6> > > ()
	<< std::endl;
	return 0;
}