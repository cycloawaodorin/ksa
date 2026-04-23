#include <Windows.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <numeric>
#include <numbers>
#include <cmath>
#include <cstring>
#include <exception>
#include <format>
#include "module2.hpp"
#include "version.hpp"

namespace KSA {

void
debug_print(std::wstring_view wstr)
{
    OutputDebugStringW(wstr.data());
}

template<typename... Args>
void
debug_print(std::wformat_string<Args...> fmt, Args&&... args)
{
	OutputDebugStringW(std::format(fmt, std::forward<Args>(args)...).c_str());
}

class Rational {
private:
	std::intmax_t numerator, denominator;
public:
	Rational(std::intmax_t num, std::intmax_t den)
	{
		if ( den == 0ll ) {
			throw std::invalid_argument("denominator must not be zero");
		}
		auto c = std::gcd(std::abs(num), std::abs(den));
		if ( den < 0ll ) {
			numerator = -num/c;
			denominator = -den/c;
		} else {
			numerator = num/c;
			denominator = den/c;
		}
	}
	template<std::integral T>
	Rational(T i) : numerator(static_cast<std::intmax_t>(i)), denominator(1ll) {}
	Rational() : numerator(0ll), denominator(1ll) {}
	Rational(float f)
	{
		constexpr static const int md=std::numeric_limits<float>::digits;
		int e;
		f = std::frexp(f, &e);
		f = std::ldexp(f, md);
		numerator = std::llrint(f);
		if ( e < md ) {
			denominator = 1ll<<(md-e);
			auto c = std::gcd(numerator, denominator);
			numerator /= c;
			denominator /= c;
		} else {
			numerator <<= e-md;
			denominator = 1ll;
		}
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
		if ( other.numerator == 0ll ) {
			throw std::invalid_argument("divisor must not be zero");
		}
		const auto ca = std::gcd(std::abs(numerator), std::abs(other.numerator));
		const auto cb = std::gcd(denominator, other.denominator);
		return Rational((numerator/ca) * (other.denominator/cb), (denominator/cb) * (other.numerator/ca));
	}
	Rational
	operator /(const std::intmax_t &other)
	const {
		if ( other == 0ll ) {
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
		if ( r < 0ll ) {
			return ( (numerator-r)/denominator - 1ll );
		} else {
			return ( (numerator-r)/denominator );
		}
	}
	std::intmax_t
	ceilm1()
	const {
		const auto r = numerator % denominator;
		if ( r <= 0ll ) {
			return ( (numerator-r)/denominator - 1ll );
		} else {
			return ( (numerator-r)/denominator );
		}
	}
	std::intmax_t
	ceil()
	const {
		const auto r = numerator % denominator;
		if ( r <= 0ll ) {
			return ( (numerator-r)/denominator );
		} else {
			return ( (numerator-r)/denominator + 1ll );
		}
	}
	std::intmax_t
	floorp1()
	const {
		const auto r = numerator % denominator;
		if ( r < 0ll ) {
			return ( (numerator-r)/denominator );
		} else {
			return ( (numerator-r)/denominator + 1ll );
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
		std::thread thread;
		std::mutex mx;
		std::condition_variable cv;
		bool ready=false;
	};
	std::size_t size;
	bool alive=true;
	std::unique_ptr<Thread[]> threads;
	std::function<void(int)> func;
	std::atomic<int> current_i=0;
	int max_i=0;
	std::exception_ptr ep;
	void
	listen(Thread *th)
	{
		while (alive) {
			{ // ジョブが来るまで待機
				auto lk=std::unique_lock(th->mx);
				th->cv.wait(lk, [th]{ return th->ready; });
			}
			for ( int i=max_i; current_i<max_i; ) { // ジョブの取り出しと実行
				i = current_i++;
				try {
					if ( i < max_i ) {
						func(i);
					}
				} catch (...) { // func からの例外を捕捉
					ep = std::current_exception();
					current_i = max_i;
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
	ThreadPool() : size(std::thread::hardware_concurrency())
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
			threads[i].cv.wait(lk, [this, i]{ return !(threads[i].ready); });
		}
		func = nullptr;
		if ( ep ) {
			std::rethrow_exception(std::exchange(ep, nullptr));
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
		}, m );
	}
};
static std::unique_ptr<ThreadPool> TP;

constexpr static const unsigned char u0=0u, u255=255u;
static unsigned char
uc_cast(float x)
{
	if ( x < 0.0f || std::isnan(x) ) {
		return u0;
	} else if ( 255.0f < x ) {
		return u255;
	} else {
		return static_cast<unsigned char>(std::nearbyint(x));
	}
}
static unsigned char
uc_cast(std::int64_t num, std::int64_t den)
{
	if ( num <= 0ll ) {
		return u0;
	} else if ( 255ll*den <= num ) {
		return u255;
	} else {
		auto r = num % den;
		if ( r*2ll < den ) {
			return static_cast<unsigned char>((num-r)/den);
		} else if ( r*2ll == den ) {
			r = (num-r)/den;
			if ( (r&1ll) == 0ll ) {
				return static_cast<unsigned char>(r);
			} else {
				return static_cast<unsigned char>(r+1ll);
			}
		} else {
			return static_cast<unsigned char>((num-r)/den+1ll);
		}
	}
}

struct PIXEL_RGBA {
	unsigned char r, g, b, a;
};
struct FloatRGBAW {
	float r, g, b, a, w;
	FloatRGBAW() : r(0.0f), g(0.0f), b(0.0f), a(0.0f), w(0.0f) {}
	void
	fma(const PIXEL_RGBA *s_px, float wxy)
	{
		const auto wxya = wxy*static_cast<float>(s_px->a);
		r = std::fma(static_cast<float>(s_px->r), wxya, r);
		g = std::fma(static_cast<float>(s_px->g), wxya, g);
		b = std::fma(static_cast<float>(s_px->b), wxya, b);
		a += wxya;
		w += wxy;
	}
	void
	put_pixel(PIXEL_RGBA *d_px)
	const {
		d_px->r = uc_cast(r/a);
		d_px->g = uc_cast(g/a);
		d_px->b = uc_cast(b/a);
		d_px->a = uc_cast(a/w);
	}
};

static bool
check_arg_num(SCRIPT_MODULE_PARAM* param, const int n)
{
	auto n_given=param->get_param_num();
	if ( n != n_given ) {
		static auto str = std::format("number of arguments must be {}, but {} given", n, n_given);
		param->set_error(str.c_str());
		return true;
	} else {
		return false;
	}
}

static void
exception_to_message(SCRIPT_MODULE_PARAM* param, std::exception &e)
{
	static auto str = std::format("exception '{}' has been thrown", e.what());
	param->set_error(str.c_str());
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
