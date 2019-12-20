#include "lua/lua.hpp"
#include <memory>
#include <thread>
#include <numeric>
#include <cmath>
#include <cstdint>
#include <functional>

namespace KSA {

class Rational {
private:
	std::int64_t numerator, denominator;
public:
	Rational(std::int64_t num, std::int64_t den)
	{
		std::int64_t c = std::gcd(std::abs(num), std::abs(den));
		if ( den < 0 ) {
			numerator = -num/c;
			denominator = -den/c;
		} else {
			numerator = num/c;
			denominator = den/c;
		}
	}
	Rational(std::int64_t i) : numerator(i), denominator(1)
	{
	}
	Rational() : numerator(0), denominator(1)
	{
	}
	std::int64_t
	get_numerator()
	const {
		return numerator;
	}
	std::int64_t
	get_denominator()
	const {
		return denominator;
	}
	Rational
	operator +(const Rational &other)
	const {
		std::int64_t c = std::gcd(denominator, other.denominator);
		std::int64_t s_d = denominator/c, o_d = other.denominator/c;
		return Rational(numerator*o_d+other.numerator*s_d, denominator*o_d);
	}
	Rational
	operator +(std::int64_t other)
	const {
		return Rational(numerator+other*denominator, denominator);
	}
	Rational
	operator -(const Rational &other)
	const {
		std::int64_t c = std::gcd(denominator, other.denominator);
		std::int64_t s_d = denominator/c, o_d = other.denominator/c;
		return Rational(numerator*o_d-other.numerator*s_d, denominator*o_d);
	}
	Rational
	operator -(std::int64_t other)
	const {
		return Rational(numerator-other*denominator, denominator);
	}
	Rational
	operator *(const Rational &other)
	const {
		std::int64_t ca = std::gcd(std::abs(numerator), other.denominator);
		std::int64_t cb = std::gcd(denominator, std::abs(other.numerator));
		return Rational((numerator/ca) * (other.numerator/cb), (denominator/cb) * (other.denominator/ca));
	}
	Rational
	operator *(std::int64_t other)
	const {
		std::int64_t c = std::gcd(std::abs(other), denominator);
		return Rational(numerator*(other/c), denominator/c);
	}
	Rational
	operator /(const Rational &other)
	const {
		std::int64_t ca = std::gcd(std::abs(numerator), std::abs(other.numerator));
		std::int64_t cb = std::gcd(denominator, other.denominator);
		return Rational((numerator/ca) * (other.denominator/cb), (denominator/cb) * (other.numerator/ca));
	}
	Rational
	operator /(std::int64_t other)
	const {
		std::int64_t c = std::gcd(std::abs(numerator), std::abs(other));
		return Rational(numerator/c, denominator*(other/c));
	}
	Rational
	reciprocal()
	const {
		return Rational(denominator, numerator);
	}
	std::int64_t
	floor()
	const {
		std::int64_t r = numerator % denominator;
		if ( r < 0 ) {
			return ( (numerator-r)/denominator - 1 );
		} else {
			return ( (numerator-r)/denominator );
		}
	}
	std::int64_t
	floor_eps()
	const {
		std::int64_t r = numerator % denominator;
		if ( r <= 0 ) {
			return ( (numerator-r)/denominator - 1 );
		} else {
			return ( (numerator-r)/denominator );
		}
	}
	std::int64_t
	ceil()
	const {
		std::int64_t r = numerator % denominator;
		if ( r <= 0 ) {
			return ( (numerator-r)/denominator );
		} else {
			return ( (numerator-r)/denominator + 1 );
		}
	}
	std::int64_t
	ceil_eps()
	const {
		std::int64_t r = numerator % denominator;
		if ( r < 0 ) {
			return ( (numerator-r)/denominator );
		} else {
			return ( (numerator-r)/denominator + 1 );
		}
	}
	float
	to_float()
	const {
		return( static_cast<float>(numerator) / static_cast<float>(denominator) );
	}
};

template <class T>
static void
parallel_do(void (*f)(T*, int, int), T *p, int n)
{
	std::unique_ptr<std::thread[]> threads(new std::thread[n]);
	for (int i=0; i<n; i++) {
		threads[i] = std::thread(f, p, i, n);
	}
	for (int i=0; i<n; i++) {
		threads[i].join();
	}
}

#include "ksa_ext.cpp"

};

static constexpr luaL_Reg ksa_ext[] = {
#include "functions.c"
	{ nullptr, nullptr }
};

extern "C" {
int
luaopen_ksa_ext(lua_State *L)
{
	luaL_register(L, "ksa_ext", ksa_ext);
	return 1;
}
}
