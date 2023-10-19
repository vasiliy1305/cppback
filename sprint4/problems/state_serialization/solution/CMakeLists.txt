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

# Добавляем библиотеку, указывая, что она статическая.
add_library(MyLib STATIC 
	src/model.h
	src/model.cpp
	src/tagged.h
	src/loot_generator.h
	src/loot_generator.cpp 
	src/model_serialization.h 
	src/geom.h
)

# Добавляем сторонние библиотеки. Указываем видимость PUBLIC, т. к. 
# они должны быть ввидны и в библиотеке MyLib и в зависимостях.
target_include_directories(MyLib PUBLIC CONAN_PKG::boost)
target_link_libraries(MyLib PUBLIC CONAN_PKG::catch2 CONAN_PKG::boost Threads::Threads)

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
	
)

add_executable(game_server_tests
	tests/state-serialization-tests.cpp
	src/collision_detector.h 
	src/collision_detector.cpp 
		src/json_loader.h
	src/json_loader.cpp
)

# Добавляем зависимость целей от статической библиотеки.
# target_include_directories уже не нужен - он добавится автоматически
# из зависимой библиотеки.
target_link_libraries(game_server MyLib)
target_link_libraries(game_server_tests ${CATCH2_LIB} MyLib) 