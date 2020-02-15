cmake_minimum_required(VERSION 3.0)
project(NarcissisticNumbers)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED true)
find_package(Threads)

add_executable(NarcissisticNumbers 
	main.cpp 
	utils.cpp
	utils.h
	NarcissisticNumCalculator.cpp
	NarcissisticNumCalculator.h
	)

target_link_libraries(NarcissisticNumbers ${CMAKE_THREAD_LIBS_INIT})
