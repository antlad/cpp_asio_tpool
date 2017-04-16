#pragma once

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/spawn.hpp>

#include <boost/hana/tuple.hpp>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <cstddef>
#include <list>
#include <tuple>
#include <type_traits>
#include <utility>
#include <iostream>

namespace atpool {

typedef boost::asio::yield_context asio_ctx;
typedef boost::asio::io_service asio_io;

class base_task {
public:
	virtual ~base_task() {}

	virtual void run() = 0;

	virtual bool is_finished() = 0;
};

class spinlock {
public:
	spinlock()
		: m_v(ATOMIC_FLAG_INIT)
	{}

	std::atomic_flag m_v;

public:

	bool try_lock()
	{
		return !m_v.test_and_set(std::memory_order_acquire);
	}

	void lock()
	{
		for (unsigned k = 0; !try_lock(); ++k)
		{
			boost::detail::yield(k);
		}
	}

	void unlock()
	{
		m_v.clear(std::memory_order_release);
	}

public:

	class scoped_lock {
	private:

		spinlock& m_sp;

		scoped_lock(scoped_lock const&);

	public:

		explicit scoped_lock(spinlock& sp): m_sp(sp)
		{
			sp.lock();
		}

		~scoped_lock()
		{
			m_sp.unlock();
		}
	};
};

template <class T, typename Functor, typename... Args>
class task:
	public base_task {
public:

	task(asio_io& io,
		 Functor&& func,
		 Args&& ... args
		)
		: m_io(io)
		, m_done(false)
		, m_f(std::move(func))
		, m_args(boost::hana::make_tuple(args...))
	{
	}

	virtual void run() override
	{
		try
		{
			m_done.store(true, std::memory_order_acquire);
			boost::hana::unpack(m_args, [this](auto ...args)
			{
				m_result = m_f(args...);
			});
		}
		catch (...)
		{
			m_exptr = std::current_exception();
		}
	}

	virtual bool is_finished() override
	{
		return m_done.load(std::memory_order_release);
	}

	T result(asio_ctx ctx)
	{
		if (m_done.load(std::memory_order_release))
		{
			if (m_exptr)
			{
				std::rethrow_exception(m_exptr);
			}

			return m_result;
		}

		boost::asio::steady_timer timer(m_io,  boost::asio::steady_timer::clock_type::duration::max());
		timer.async_wait(ctx);

		if (m_exptr)
		{
			std::rethrow_exception(m_exptr);
		}

		return m_result;
	}

private:
	asio_io& m_io;
	T m_result;
	std::atomic<bool> m_done;
	std::exception_ptr m_exptr;
	std::function<T(Args...)> m_f;
	boost::hana::tuple<Args...> m_args;
};


class thread_pool {
public:
	thread_pool(asio_io& io,
				std::size_t thread_count = std::thread::hardware_concurrency(),
				int inactive_time_sleep_microsecs = 50000)
		: m_io(io)
		, m_stop(false)
		, m_inactive_time_sleep_microsecs(inactive_time_sleep_microsecs)
	{
		for (size_t i = 0; i < thread_count; ++i)
		{
			m_threads.emplace_back(&thread_pool::thread_executor, this);
		}
	}

	void thread_executor()
	{
		std::shared_ptr<base_task> task_ptr;
		bool need_sleep = false;

		while (m_stop.load(std::memory_order_relaxed) != true)
		{
			task_ptr.reset();
			{
				spinlock::scoped_lock g(m_inputlock);

				if (!m_input_tasks.empty())
				{
					task_ptr = m_input_tasks.back();
					m_input_tasks.pop_back();
					need_sleep = false;
				}
				else
				{
					need_sleep = true;
				}
			}

			if (task_ptr)
			{
				task_ptr->run();
			}

			if (need_sleep)
			{
				std::this_thread::sleep_for(std::chrono::microseconds(m_inactive_time_sleep_microsecs));
			}
		}
	}


	template <typename Functor, typename... Args>
	auto run(Functor&& f, Args&& ... args)
	{
		auto t = std::make_shared<task<typename std::result_of<Functor(Args...)>::type, Functor, Args...>>
				 (m_io, std::forward<Functor>(f), std::forward<Args>(args)...);
		spinlock::scoped_lock g(m_inputlock);
		m_input_tasks.push_front(t);
		return t;
	}

	virtual ~thread_pool()
	{
		wait_all();
	}

	void wait_all()
	{
		m_stop.store(true, std::memory_order_relaxed);

		for (auto& t : m_threads)
		{
			try
			{
				t.join();
			}
			catch (const std::system_error& e)
			{
			}
		}
	}

private:
	asio_io& m_io;
	std::atomic<bool> m_stop;
	int m_inactive_time_sleep_microsecs;

	spinlock m_inputlock;
	spinlock m_outputlock;

	std::list<std::shared_ptr<base_task>> m_input_tasks;
	std::list<std::shared_ptr<base_task>> m_output_tasks;
	std::vector<std::thread> m_threads;
};


}
