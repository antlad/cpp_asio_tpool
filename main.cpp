//#include "threadpool.h"

#include "asio_tpool.hpp"

#include <iostream>
#include <functional>
#include <thread>
#include <chrono>

#include <boost/asio/system_timer.hpp>
#include <boost/exception/exception.hpp>

int some_long_op(int a, int b)
{
	std::cout << "Boom !" << std::endl;
	std::cout.flush();
	std::this_thread::sleep_for(std::chrono::seconds(1));
	BOOST_THROW_EXCEPTION(std::runtime_error("EXIT in run!"));
	return a + b;
}


void main_async(atpool::io& io, atpool::ctx ctx)
{
	atpool::thread_pool tpool(io);
	std::cout << "async start thread id = " << std::this_thread::get_id() << std::endl;
	boost::asio::spawn(io, [&io](atpool::ctx ctx)
	{
		int i = 0;

		for (;;)
		{
			boost::asio::system_timer timer(io);
			timer.expires_from_now(std::chrono::milliseconds(1000));
			timer.async_wait(ctx);
			std::cout << i++ << " secs. tick 1000 ms thread id = " << std::this_thread::get_id() << std::endl;
		}
	});
	auto t = tpool.run(&some_long_op, 11, 22);

	try
	{
		std::cout << t->result(ctx) << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	tpool.wait_all();
	std::cout << "main finished" << std::endl;
}


int main(int argc, char* argv[])
{
	try
	{
		atpool::io io;
		boost::asio::spawn(io, [&](atpool::ctx ctx)
		{
			main_async(io, ctx);
		});
		io.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << boost::current_exception_diagnostic_information(true) << std::endl;
		exit(1);
	}
	catch (...)
	{
		std::cerr << "unknown exception, aborting" << std::endl;
		exit(1);
	}

	return 0;
}
