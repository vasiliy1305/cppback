cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

get_property(importTargets DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY IMPORTED_TARGETS)
message(STATUS "${importTargets}") 

find_package(Boost 1.78.0 REQUIRED)

include(CTest)
include(${CONAN_BUILD_DIRS_CATCH2}/Catch.cmake) 

add_compile_definitions(BOOST_BEAST_USE_STD_STRING_VIEW)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(game_server_static_lib
src/model.h
src/model.cpp
src/tagged.h
src/loot_generator.h
src/loot_generator.cpp 
src/geom.h
  )

target_link_libraries(game_server_static_lib PUBLIC CONAN_PKG::boost)

add_executable(game_server
	src/main.cpp
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
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
	src/collision_detector.h 
	src/collision_detector.cpp 
	src/file_loader.h 
	src/file_loader.cpp 
	src/database.h 
	src/database.cpp 
	src/tagged_uuid.h 
	src/tagged_uuid.cpp 
	
)

# add_executable(game_server_tests
# 	tests/state-serialization-tests.cpp
# 	src/collision_detector.h 
# 	src/collision_detector.cpp 
# 	src/json_loader.h
# 	src/json_loader.cpp
# 	src/file_loader.h 
# 	src/file_loader.cpp
# )

target_link_libraries(game_server PRIVATE Threads::Threads)
target_link_libraries(game_server PRIVATE CONAN_PKG::boost)
target_link_libraries(game_server PRIVATE game_server_static_lib)
target_link_libraries(game_server PUBLIC CONAN_PKG::boost Threads::Threads CONAN_PKG::libpq CONAN_PKG::libpqxx)

# target_link_libraries(game_server_tests PRIVATE CONAN_PKG::catch2 CONAN_PKG::boost Threads::Threads game_server_static_lib) 

# catch_discover_tests(game_server_tests) 