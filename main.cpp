//#include "threadpool.h"

//#include "asio_tpool.hpp"



#include <iostream>
#include <functional>
#include <thread>
#include <chrono>



int main(int argc, char* argv[])
{
	try
	{
	}
	catch (const std::exception& e)
	{
		//std::cerr << boost::current_exception_diagnostic_information(true) << std::endl;
		exit(1);
	}
	catch (...)
	{
		std::cerr << "unknown exception, aborting" << std::endl;
		exit(1);
	}

	return 0;
}
