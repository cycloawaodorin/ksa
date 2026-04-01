#include "lua/lua.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <numeric>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace KSA {

class Rational {
private:
	std::intmax_t numerator, denominator;
public:
	Rational(const std::intmax_t &num, const std::intmax_t &den)
	{
		if ( den == 0 ) {
			throw std::invalid_argument("denominator must not be zero");
		}
		auto c = std::gcd(std::abs(num), std::abs(den));
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
		const auto c = std::gcd(denominator, other.denominator);
		const auto s_d = denominator/c, o_d = other.denominator/c;
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
		const auto c = std::gcd(denominator, other.denominator);
		const auto s_d = denominator/c, o_d = other.denominator/c;
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
		const auto ca = std::gcd(std::abs(numerator), other.denominator);
		const auto cb = std::gcd(denominator, std::abs(other.numerator));
		return Rational((numerator/ca) * (other.numerator/cb), (denominator/cb) * (other.denominator/ca));
	}
	Rational
	operator *(const std::intmax_t &other)
	const {
		const auto c = std::gcd(std::abs(other), denominator);
		return Rational(numerator*(other/c), denominator/c);
	}
	Rational
	operator /(const Rational &other)
	const {
		if ( other.numerator == 0 ) {
			throw std::invalid_argument("divisor must not be zero");
		}
		const auto ca = std::gcd(std::abs(numerator), std::abs(other.numerator));
		const auto cb = std::gcd(denominator, other.denominator);
		return Rational((numerator/ca) * (other.denominator/cb), (denominator/cb) * (other.numerator/ca));
	}
	Rational
	operator /(const std::intmax_t &other)
	const {
		if ( other == 0 ) {
			throw std::invalid_argument("divisor must not be zero");
		}
		const auto c = std::gcd(std::abs(numerator), std::abs(other));
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
		const auto r = numerator % denominator;
		if ( r < 0 ) {
			return ( (numerator-r)/denominator - 1 );
		} else {
			return ( (numerator-r)/denominator );
		}
	}
	std::intmax_t
	floor_eps()
	const {
		const auto r = numerator % denominator;
		if ( r <= 0 ) {
			return ( (numerator-r)/denominator - 1 );
		} else {
			return ( (numerator-r)/denominator );
		}
	}
	std::intmax_t
	ceil()
	const {
		const auto r = numerator % denominator;
		if ( r <= 0 ) {
			return ( (numerator-r)/denominator );
		} else {
			return ( (numerator-r)/denominator + 1 );
		}
	}
	std::intmax_t
	ceil_eps()
	const {
		const auto r = numerator % denominator;
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

class ThreadPool {
private:
	struct Thread {
		bool ready;
		std::thread thread;
		std::mutex mx;
		std::condition_variable cv;
		Thread() : ready(false) {}
	};
	std::size_t size;
	bool alive;
	std::unique_ptr<Thread[]> threads;
	std::function<void(int)> func;
	std::mutex gmx;
	std::atomic<int> current_i;
	int max_i;
	void
	listen(Thread *th)
	{
		while (alive) {
			{ // ジョブが来るまで待機
				auto lk=std::unique_lock(th->mx);
				th->cv.wait(lk, [&]{ return th->ready; });
			}
			for ( int i=max_i; current_i<max_i; ) { // ジョブの取り出しと実行
				i = current_i++;
				if ( i < max_i ) {
					func(i);
				}
			}
			{ // 全ジョブ完了
				auto lk=std::lock_guard(th->mx);
				th->ready = false;
			}
			th->cv.notify_one();
		}
	}
public:
	ThreadPool() : size(std::thread::hardware_concurrency()), alive(true)
	{
		threads = std::make_unique<Thread[]>(size);
		for (auto i=0uz; i<size; i++) {
			threads[i].thread = std::thread([this, i](){listen(&threads[i]);});
		}
	}
	~ThreadPool()
	{
		{
			alive = false;
			for (auto i=0uz; i<size; i++) {
				{
					auto lk=std::lock_guard(threads[i].mx);
					threads[i].ready = true;
				}
				threads[i].cv.notify_one();
			}
		}
		for (auto i=0uz; i<size; i++) {
			threads[i].thread.join();
		}
		func = nullptr;
	}
	void
	parallel_do(std::function<void(int)> f, int n)
	{
		func = f; // ジョブ関数
		current_i = 0; max_i = n;
		for (auto i=0uz; i<size; i++) { // ワーカー起動
			{
				auto lk=std::lock_guard(threads[i].mx);
				threads[i].ready = true;
			}
			threads[i].cv.notify_one();
		}
		for (auto i=0uz; i<size; i++) { // 全ワーカーの終了を待つ
			auto lk=std::unique_lock(threads[i].mx);
			threads[i].cv.wait(lk, [&]{ return !(threads[i].ready); });
		}
	}
	void
	parallel_do_batched(std::function<void(int)> f, int n)
	{
		const int m = static_cast<int>(size);
		parallel_do( [&f, n, m](int i){
			const int s=(i*n)/m, e=((i+1)*n)/m;
			for (auto j=s; j<e; j++) {
				f(j);
			}
		}, n );
	}
};
static std::unique_ptr<ThreadPool> TP;

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
		return static_cast<unsigned char>(std::nearbyint(x));
	}
}
static unsigned char
uc_cast(std::uint32_t num, std::uint32_t den)
{
	if ( num == 0u ) {
		return static_cast<unsigned char>(0u);
	} else if ( 255u*den <= num ) {
		return static_cast<unsigned char>(255u);
	} else {
		auto r = num % den;
		if ( r*2u < den ) {
			return static_cast<unsigned char>((num-r)/den);
		} else if ( r*2u == den ) {
			r = (num-r)/den;
			if ( r%2u == 0u ) {
				return static_cast<unsigned char>(r);
			} else {
				return static_cast<unsigned char>(r+1);
			}
		} else {
			return static_cast<unsigned char>((num-r)/den+1);
		}
	}
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
	KSA::TP = std::make_unique<KSA::ThreadPool>();
	luaL_register(L, "ksa_ext", ksa_ext);
	return 1;
}
}
