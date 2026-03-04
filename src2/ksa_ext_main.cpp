#include <Windows.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <numeric>
#include <cmath>
#include <cstring>
#include "module2.hpp"
#include "version.hpp"

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

class ThreadPool {
private:
	class Thread {
	public:
		std::thread thread;
		bool ready;
		std::mutex mx;
		std::condition_variable cv;
		Thread() : ready(false) {}
	};
	std::unique_ptr<Thread[]> threads;
	int size;
	std::function<void(int)> func;
	std::mutex gmx;
	std::queue<int> jobs;
	bool terminate;
	void
	listen(Thread *th)
	{
		for (;;) {
			{ // ジョブが来るまで待機
				std::unique_lock<std::mutex> lk(th->mx);
				th->cv.wait(lk, [&]{ return th->ready; });
			}
			if ( terminate ) { // スレッドプールの破棄
				return;
			}
			for (int i=INT_MAX; !jobs.empty();) {
				{ // ジョブの取り出し
					std::lock_guard<std::mutex> lk(gmx);
					if ( !jobs.empty() ) {
						i = jobs.front();
						jobs.pop();
					}
				}
				if ( i < INT_MAX ) { // ジョブ実行
					func(i);
				}
			}
			{ // 全ジョブ完了
				std::lock_guard<std::mutex> lk(th->mx);
				th->ready = false;
			}
			th->cv.notify_one();
		}
	}
public:
	ThreadPool() : size(std::thread::hardware_concurrency()), terminate(false)
	{
		threads = std::make_unique<Thread[]>(size);
		for (auto i=0; i<size; i++) {
			threads[i].thread = std::thread(listen, this, &threads[i]);
		}
	}
	~ThreadPool()
	{
		{
			for (auto i=0; i<size; i++) {
				threads[i].mx.lock();
				threads[i].ready = true;
			}
			terminate = true;
			for (auto i=0; i<size; i++) {
				threads[i].mx.unlock();
				threads[i].cv.notify_all();
			}
		}
		for (auto i=0; i<size; i++) {
			threads[i].thread.join();
		}
	}
	void
	parallel_do(std::function<void(int)> f, int n)
	{
		func = f; // ジョブ関数
		for (int i=0; i<n; i++) {
			jobs.push(i); // ジョブ引数をセット
		}
		for (auto i=0; i<size; i++) { // ワーカー起動
			{
				std::lock_guard<std::mutex> lk(threads[i].mx);
				threads[i].ready = true;
			}
			threads[i].cv.notify_one();
		}
		for (auto i=0; i<size; i++) { // 全ワーカーの終了を待つ
			std::unique_lock<std::mutex> lk(threads[i].mx);
			threads[i].cv.wait(lk, [&]{ return !(threads[i].ready); });
		}
	}
};
static std::unique_ptr<ThreadPool> TP;

constexpr static const float PI = 3.141592653589793f;
struct PIXEL_RGBA {
	unsigned char r, g, b, a;
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
uc_cast(std::intmax_t num, std::intmax_t den)
{
	std::intmax_t c = std::gcd(std::abs(num), std::abs(den));
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
		std::intmax_t r = num % den;
		if ( r*2 < den ) {
			return static_cast<unsigned char>((num-r)/den);
		} else {
			return static_cast<unsigned char>((num-r)/den+1);
		}
	}
}

static bool
check_arg_num(SCRIPT_MODULE_PARAM* param, const int n)
{
	auto n_given=param->get_param_num();
	if ( n != n_given ) {
		static std::string str=std::format("number of arguments must be {}, but {} given", n, n_given);
		param->set_error(str.c_str());
		return true;
	} else {
		return false;
	}
}

#include "ksa_ext.cpp"

};

static SCRIPT_MODULE_FUNCTION ksa_ext[] = {
#include "functions.cpp"
	{ nullptr, nullptr }
};


EXTERN_C SCRIPT_MODULE_TABLE *
GetScriptModuleTable()
{
	static SCRIPT_MODULE_TABLE smt = {
		L"KSA Extention Module Version " VERSION L" by KAZOON",
		ksa_ext
	};
	return &smt;
}

EXTERN_C bool
InitializePlugin(DWORD version)
{
	KSA::TP = std::make_unique<KSA::ThreadPool>();
	return true;
}

EXTERN_C void
UninitializePlugin()
{
	KSA::TP.reset(nullptr);
}
