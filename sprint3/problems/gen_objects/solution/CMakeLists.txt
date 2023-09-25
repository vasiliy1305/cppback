cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(game_server
	src/main.cpp
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/model.h
	src/model.cpp
	src/tagged.h
	src/boost_json.cpp
	src/json_loader.h
	src/json_loader.cpp
	src/request_handler.cpp
	src/request_handler.h
	src/tests.h
	src/application.h 
	src/application.cpp
	src/logger.h 
	src/logger.cpp
	src/loot_generator.h
	src/loot_generator.cpp 
)

add_executable(test_loot
	tests/loot_generator_tests.cpp
	# src/http_server.cpp
	# src/http_server.h
	# src/sdk.h
	# src/model.h
	# src/model.cpp
	# src/tagged.h
	# src/boost_json.cpp
	# src/json_loader.h
	# src/json_loader.cpp
	# src/request_handler.cpp
	# src/request_handler.h
	# src/tests.h
	# src/application.h 
	# src/application.cpp
	# src/logger.h 
	# src/logger.cpp
	src/loot_generator.h
	src/loot_generator.cpp 
)

target_link_libraries(test_loot PRIVATE CONAN_PKG::catch2)
target_include_directories(game_server PRIVATE CONAN_PKG::boost)
target_link_libraries(game_server CONAN_PKG::boost)
target_include_directories(game_server PRIVATE ${Boost_INCLUDE_DIRS})