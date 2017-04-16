project(unit_tests)
enable_testing()
find_package(GTest REQUIRED)

function(add_test_helper test_target_name internal_test_name test_src_list )
	add_executable(${test_target_name} ${test_src_list})
	target_link_libraries(${test_target_name} GTest::GTest GTest::Main ${Boost_LIBRARIES})
	add_test(${internal_test_name} ${test_target_name})
endfunction(add_test_helper)


add_test_helper(simple_run SimpleRun simple_run.cpp)
