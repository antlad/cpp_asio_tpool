#include "asio_tpool.hpp"

#include <gtest/gtest.h>



TEST(SimpleRun, simple_run)
{
	atpool::io io;
	boost::asio::spawn(io, [&](atpool::ctx ctx)
	{
		atpool::thread_pool tpool(io);
		auto t = tpool.run([](int a, int b)
		{
			throw std::runtime_error("BOOM!");
			return a + b;
		}, 2, 2);
		EXPECT_THROW(t->result(ctx), std::runtime_error);
	});
	io.run_one();
}

TEST(SimpleRun, case0)
{
	atpool::io io;
	boost::asio::spawn(io, [&](atpool::ctx ctx)
	{
		atpool::thread_pool tpool(io);
		auto t = tpool.run([](int a, int b)
		{
			return a + b;
		}, 2, 2);
		ASSERT_EQ(t->result(ctx), 4);
	});
	io.run_one();
}
