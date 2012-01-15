cmake_minimum_required(VERSION 2.6)

project(fast_net_server)

add_definitions(-g -pipe -Wall -march=native -O3)
# add_definitions(-g -pipe -Wall -march=pentium4 -O3 -funroll-loops)
# add_definitions(-g -pipe -Wall)

add_definitions(-DTHREAD_POOL_DEBUG)

add_definitions(-D_GNU_SOURCE)

if(DO_DEBUG)
	add_definitions(-fno-inline -DNO_INLINE)
endif(DO_DEBUG)

add_executable(test_workq workq.c test_workq.c)
add_executable(test_threads workq.c thread_pool.c test_threads.c)
target_link_libraries(test_threads pthread)

# target_link_libraries(getipv6addr-proc bz2)
