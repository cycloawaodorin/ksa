#include "lua/lua.hpp"

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace KSA {

class ThreadPool {
private:
	class Thread {
	public:
		enum class State { NotInitialized, JobItemPrepared, JobFinished };
		ThreadPool *pool;
		std::thread thread;
		State state;
		Thread() : state(State::NotInitialized)
		{
		}
		void
		listen(const std::size_t id)
		{
			for (;;) {
				{
					std::unique_lock<std::mutex> lk(pool->mx);
					pool->cv.wait(lk, [&]{ return (state==State::JobItemPrepared); });
				}
				if ( pool->terminate ) {
					return;
				}
				pool->func(pool->data, id, pool->n);
				{
					std::lock_guard<std::mutex> lk(pool->mx);
					state = State::JobFinished;
				}
				pool->cv.notify_all();
			}
		}
	};
	std::unique_ptr<Thread[]> threads;
	std::size_t size;
	std::mutex mx;
	std::condition_variable cv;
	std::function<void(void *, std::size_t, std::size_t)> func;
	void *data;
	std::size_t n;
	bool terminate;
public:
	ThreadPool() : size(std::thread::hardware_concurrency()), terminate(false)
	{
		threads.reset(new Thread[size]);
		for (std::size_t i=0; i<size; i++) {
			threads[i].pool = this;
			threads[i].thread = std::thread(Thread::listen, &threads[i], i);
		}
	}
	~ThreadPool()
	{
		{
			std::lock_guard<std::mutex> lk(mx);
			for (std::size_t i=0; i<size; i++) {
				threads[i].state = Thread::State::JobItemPrepared;
			}
			terminate = true;
		}
		cv.notify_all();
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
		{
			std::lock_guard<std::mutex> lk(mx);
			for (std::size_t i=0; i<n; i++) {
				threads[i].state = Thread::State::JobItemPrepared;
			}
		}
		cv.notify_all();
		std::unique_lock<std::mutex> lk(mx);
		cv.wait(lk, [&]{
			for (std::size_t i=0; i<n; i++) {
				if ( threads[i].state != Thread::State::JobFinished ) {
					return false;
				}
			}
			return true;
		});
	}
};
static std::unique_ptr<ThreadPool> TP;

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
