#include "lua/lua.hpp"

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <windows.h>
#include <stdlib.h>
#   define MyOutputDebugString( str, ... ) \
      { \
        TCHAR __odstr[256]; \
        sprintf( __odstr, str, __VA_ARGS__ ); \
        OutputDebugString( __odstr ); \
      }

namespace KSA {

class ThreadPool {
private:
	class Thread {
	public:
		std::thread thread;
		std::mutex mx;
		std::condition_variable cv;
		std::function<void(void *, std::size_t, std::size_t)> func;
		void *data;
		std::size_t n;
		bool terminate;
		bool ready;
		bool finished;
		Thread() : terminate(false), ready(false), finished(false)
		{
		}
		void
		listen(const std::size_t id)
		{
			for (;;) {
				{
					std::unique_lock<std::mutex> lk(mx);
					cv.wait(lk, [&]{ return ready; });
				}
				if ( terminate ) {
					return;
				} else {
					func(data, id, n);
				}
				{
					std::lock_guard<std::mutex> lk(mx);
					ready = false;
					finished = true;
				}
				cv.notify_all();
			}
		}
	};
	std::unique_ptr<Thread[]> threads;
	std::size_t size;
public:
	ThreadPool() : size(std::thread::hardware_concurrency())
	{
		threads.reset(new Thread[size]);
		for (std::size_t i=0; i<size; i++) {
			threads[i].thread = std::thread(Thread::listen, &threads[i], i);
		}
	}
	~ThreadPool()
	{
		for (std::size_t i=0; i<size; i++) {
			{
				std::lock_guard<std::mutex> lk(threads[i].mx);
				threads[i].ready = true;
				threads[i].terminate = true;
			}
			threads[i].cv.notify_all();
			threads[i].thread.detach();
		}
	}
	
	void
	invoke(std::function<void(void *, std::size_t, std::size_t)> f, void *data, std::size_t n)
	{
		if ( size < n ) {
			n = size;
		}
		for (std::size_t i=0; i<n; i++) {
			threads[i].func = f;
			threads[i].data = data;
			threads[i].n = n;
			{
				std::lock_guard<std::mutex> lk(threads[i].mx);
				threads[i].ready = true;
				threads[i].finished = false;
			}
			threads[i].cv.notify_all();
		}
		for (std::size_t i=0; i<n; i++) {
			std::unique_lock<std::mutex> lk(threads[i].mx);
			threads[i].cv.wait(lk, [&]{return threads[i].finished;});
		}
	}
};
static std::unique_ptr<ThreadPool> TP;
static int
ksa_exit(lua_State *L)
{
	TP.reset(nullptr);
	return 0;
}
};

#include "ksa_ext.cpp"

static constexpr luaL_Reg ksa_ext[] = {
#include "functions.c"
	{ "exit", KSA::ksa_exit },
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
