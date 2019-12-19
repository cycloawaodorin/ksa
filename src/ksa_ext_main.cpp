#include "lua/lua.hpp"

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <numeric>

namespace KSA {

class ThreadPool {
private:
	enum class State { NotInitialized, JobPrepared, JobFinished };
	class Thread {
	public:
		std::thread thread;
		State state;
		std::mutex mx;
		std::condition_variable cv;
		Thread() : state(State::NotInitialized)
		{
		}
	};
	std::unique_ptr<Thread[]> threads;
	std::size_t size;
	std::function<void(void *, std::size_t, std::size_t)> func;
	void *data;
	std::size_t n;
	bool terminate;
	void
	listen(Thread *th, const std::size_t id)
	{
		for (;;) {
			{
				std::unique_lock<std::mutex> lk(th->mx);
				th->cv.wait(lk, [&]{ return (th->state==State::JobPrepared); });
			}
			if ( terminate ) {
				return;
			}
			func(data, id, n);
			{
				std::lock_guard<std::mutex> lk(th->mx);
				th->state = State::JobFinished;
			}
			th->cv.notify_one();
		}
	}
public:
	ThreadPool() : size(std::thread::hardware_concurrency()), terminate(false)
	{
		threads.reset(new Thread[size]);
		for (std::size_t i=0; i<size; i++) {
			threads[i].thread = std::thread(listen, this, &threads[i], i);
		}
	}
	~ThreadPool()
	{
		{
			for (std::size_t i=0; i<size; i++) {
				threads[i].mx.lock();
				threads[i].state = State::JobPrepared;
			}
			terminate = true;
			for (std::size_t i=0; i<size; i++) {
				threads[i].mx.unlock();
				threads[i].cv.notify_all();
			}
		}
		for (std::size_t i=0; i<size; i++) {
			threads[i].thread.detach();
		}
	}
	
	void
	invoke(std::function<void(void *, std::size_t, std::size_t)> f, void *p, std::size_t m)
	{
		func = f;
		data = p;
		if ( size < m ) {
			n = size;
		} else {
			n = m;
		}
		for (std::size_t i=0; i<n; i++) {
			{
				std::lock_guard<std::mutex> lk(threads[i].mx);
				threads[i].state = State::JobPrepared;
			}
			threads[i].cv.notify_one();
		}
		for (std::size_t i=0; i<n; i++) {
			std::unique_lock<std::mutex> lk(threads[i].mx);
			threads[i].cv.wait(lk, [&]{ return (threads[i].state==State::JobFinished); });
		}
	}
};
static std::unique_ptr<ThreadPool> TP;

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

};

#include "ksa_ext.cpp"

static constexpr luaL_Reg ksa_ext[] = {
#include "functions.c"
	{ nullptr, nullptr }
};

extern "C" {
int
luaopen_ksa_ext(lua_State *L)
{
	KSA::TP.reset(new KSA::ThreadPool());
	luaL_register(L, "ksa_ext", ksa_ext);
	return 1;
}
}
