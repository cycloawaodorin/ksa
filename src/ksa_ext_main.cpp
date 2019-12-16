#include "lua/lua.hpp"

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace KSA {

class ThreadPool {
private:
	enum class State { NotInitialized, JobItemPrepared, JobFinished };
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
				th->cv.wait(lk, [&]{ return (th->state==State::JobItemPrepared); });
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
				threads[i].state = State::JobItemPrepared;
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
				threads[i].state = State::JobItemPrepared;
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
