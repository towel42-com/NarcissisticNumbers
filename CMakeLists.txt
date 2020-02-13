project(NarcissisticNumbers)
add_executable(NarcissisticNumbers 
	main.cpp 
	utils.cpp
	utils.h
	NarcissisticNumCalculator.cpp
	NarcissisticNumCalculator.h
	)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED true)
