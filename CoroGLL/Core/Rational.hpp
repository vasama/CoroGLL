#pragma once

#include <numeric>
#include <ostream>

#include "Types.hpp"

namespace CoroGLL {

struct Rational
{
	Rational()
		: Rational(0, 1)
	{
	}

	Rational(i64 i)
		: Rational(i, 1)
	{
	}

	Rational(i64 n, i64 d)
		: n(n), d(d)
	{
		Simplify();
	}

	Rational(Rational&&) = default;
	Rational& operator=(Rational&&) = default;

	Rational(const Rational&) = default;
	Rational& operator=(const Rational&) = default;

	i64 GetNumerator() const
	{
		return n;
	}

	void SetNumerator(i64 i)
	{
		n = i;
	}

	i64 Denominator() const
	{
		return d;
	}

	void SetDenominator(i64 i)
	{
		d = i;
	}

	Rational& operator+=(i64 i)
	{
		n += i * d;
		Simplify();
		return *this;
	}

	Rational& operator-=(i64 i)
	{
		n -= i * d;
		Simplify();
		return *this;
	}

	Rational& operator*=(i64 i)
	{
		n *= i;
		Simplify();
		return *this;
	}

	Rational& operator/=(i64 i)
	{
		d *= i;
		Simplify();
		return *this;
	}

private:
	void Simplify()
	{
		i64 gcd = std::gcd(n, d);
		n /= gcd;
		d /= gcd;
	}

	i64 n;
	i64 d;
};

} // namespace CoroGLL

inline CoroGLL::Rational operator+(const CoroGLL::Rational& a, CoroGLL::i64 b)
{
	CoroGLL::Rational r = a;
	return r += b;
}

inline CoroGLL::Rational operator-(const CoroGLL::Rational& a, CoroGLL::i64 b)
{
	CoroGLL::Rational r = a;
	return r -= b;
}

inline CoroGLL::Rational operator*(const CoroGLL::Rational& a, CoroGLL::i64 b)
{
	CoroGLL::Rational r = a;
	return r *= b;
}

inline CoroGLL::Rational operator/(const CoroGLL::Rational& a, CoroGLL::i64 b)
{
	CoroGLL::Rational r = a;
	return r /= b;
}

inline std::ostream& operator<<(std::ostream& os, const CoroGLL::Rational& r)
{
	CoroGLL::i64 d = r.Denominator();
	os << r.GetNumerator();
	if (d != 1) os << '/' << d;
	return os;
}
