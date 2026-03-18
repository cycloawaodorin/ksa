#include "lua/lua.hpp"
#include <thread>
#include <numeric>
#include <cmath>
#include <cstring>

namespace KSA {

class Rational {
private:
	std::intmax_t numerator, denominator;
public:
	Rational(const std::intmax_t &num, const std::intmax_t &den)
	{
		std::intmax_t c = std::gcd(std::abs(num), std::abs(den));
		if ( den < 0 ) {
			numerator = -num/c;
			denominator = -den/c;
		} else {
			numerator = num/c;
			denominator = den/c;
		}
	}
	Rational(const std::intmax_t &i) : numerator(i), denominator(1)
	{
	}
	Rational() : numerator(0), denominator(1)
	{
	}
	std::intmax_t
	get_numerator()
	const {
		return numerator;
	}
	std::intmax_t
	get_denominator()
	const {
		return denominator;
	}
	Rational
	operator +(const Rational &other)
	const {
		const std::intmax_t c = std::gcd(denominator, other.denominator);
		const std::intmax_t s_d = denominator/c, o_d = other.denominator/c;
		return Rational(numerator*o_d+other.numerator*s_d, denominator*o_d);
	}
	Rational
	operator +(const std::intmax_t &other)
	const {
		return Rational(numerator+other*denominator, denominator);
	}
	Rational
	operator -(const Rational &other)
	const {
		const std::intmax_t c = std::gcd(denominator, other.denominator);
		const std::intmax_t s_d = denominator/c, o_d = other.denominator/c;
		return Rational(numerator*o_d-other.numerator*s_d, denominator*o_d);
	}
	Rational
	operator -(const std::intmax_t &other)
	const {
		return Rational(numerator-other*denominator, denominator);
	}
	Rational
	operator *(const Rational &other)
	const {
		const std::intmax_t ca = std::gcd(std::abs(numerator), other.denominator);
		const std::intmax_t cb = std::gcd(denominator, std::abs(other.numerator));
		return Rational((numerator/ca) * (other.numerator/cb), (denominator/cb) * (other.denominator/ca));
	}
	Rational
	operator *(const std::intmax_t &other)
	const {
		const std::intmax_t c = std::gcd(std::abs(other), denominator);
		return Rational(numerator*(other/c), denominator/c);
	}
	Rational
	operator /(const Rational &other)
	const {
		const std::intmax_t ca = std::gcd(std::abs(numerator), std::abs(other.numerator));
		const std::intmax_t cb = std::gcd(denominator, other.denominator);
		return Rational((numerator/ca) * (other.denominator/cb), (denominator/cb) * (other.numerator/ca));
	}
	Rational
	operator /(const std::intmax_t &other)
	const {
		const std::intmax_t c = std::gcd(std::abs(numerator), std::abs(other));
		return Rational(numerator/c, denominator*(other/c));
	}
	Rational
	reciprocal()
	const {
		return Rational(denominator, numerator);
	}
	std::intmax_t
	floor()
	const {
		const std::intmax_t r = numerator % denominator;
		if ( r < 0 ) {
			return ( (numerator-r)/denominator - 1 );
		} else {
			return ( (numerator-r)/denominator );
		}
	}
	std::intmax_t
	floor_eps()
	const {
		const std::intmax_t r = numerator % denominator;
		if ( r <= 0 ) {
			return ( (numerator-r)/denominator - 1 );
		} else {
			return ( (numerator-r)/denominator );
		}
	}
	std::intmax_t
	ceil()
	const {
		const std::intmax_t r = numerator % denominator;
		if ( r <= 0 ) {
			return ( (numerator-r)/denominator );
		} else {
			return ( (numerator-r)/denominator + 1 );
		}
	}
	std::intmax_t
	ceil_eps()
	const {
		const std::intmax_t r = numerator % denominator;
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
parallel_do(void (T::*f)(int, const int &), T *p, const int &n)
{
	if ( 1 < n ) {
		auto threads=std::make_unique<std::thread[]>(n);
		for (int i=0; i<n; i++) {
			threads[i] = std::thread(f, p, i, n);
		}
		for (int i=0; i<n; i++) {
			threads[i].join();
		}
	} else {
		(p->*f)(0, n);
	}
}

constexpr static const float PI = 3.141592653589793f;
struct PIXEL_BGRA {
	alignas(1) unsigned char b;
	alignas(1) unsigned char g;
	alignas(1) unsigned char r;
	alignas(1) unsigned char a;
};
static unsigned char
uc_cast(const float &x)
{
	if ( x < 0.0f || std::isnan(x) ) {
		return static_cast<unsigned char>(0);
	} else if ( 255.0f < x ) {
		return static_cast<unsigned char>(255);
	} else {
		return static_cast<unsigned char>(std::round(x));
	}
}
static unsigned char
uc_cast(int num, int den)
{
	auto c = std::gcd(std::abs(num), std::abs(den));
	if ( den < 0 ) {
		num = -num/c;
		den = -den/c;
	} else {
		num = num/c;
		den = den/c;
	}
	if ( num <= 0 ) {
		return static_cast<unsigned char>(0);
	} else if ( 255*den <= num ) {
		return static_cast<unsigned char>(255);
	} else {
		auto r = num % den;
		if ( r*2 < den ) {
			return static_cast<unsigned char>((num-r)/den);
		} else {
			return static_cast<unsigned char>((num-r)/den+1);
		}
	}
}
static int
n_th_correction(int n_th)
{
	if ( n_th <= 0 ) {
		n_th += std::thread::hardware_concurrency();
		if ( n_th <= 0 ) {
			n_th = 1;
		}
	}
	return n_th;
}

#include "ksa_ext.cpp"

};

constexpr static const luaL_Reg ksa_ext[] = {
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
