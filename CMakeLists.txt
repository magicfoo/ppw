cmake_minimum_required(VERSION 3.5)

get_directory_property(hasParent PARENT_DIRECTORY)

project(PPW)

set(PPW_VERSION_MAJOR "0")
set(PPW_VERSION_MINOR "1")
set(PPW_VERSION "${PPW_VERSION_MAJOR}.${PPW_VERSION_MINOR}")

set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DNOMINMAX /EHsc /W2 /WX /arch:AVX")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wconversion -Wno-array-bounds")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2 -mf16c -m64")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -D__STDC_FORMAT_MACROS")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O3")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O3")
endif()

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(PPW_BUILD_EXAMPLES "Build the PPW example programs" ON)
option(PPW_BUILD_TESTS "Build the PPW test programs" ON)

set(PPW_INC "${PPW_SOURCE_DIR}/api")
set(PPW_CPP "PPW")

if(hasParent)
	set(PPW_LIBS
		ppw::ipc_core
		ppw::fm_core
		PARENT_SCOPE)
endif()

set(fm_core
	code/fatmodel/fm_core.cpp
	code/fatmodel/fm_utils.h
	code/fatmodel/fm_utils.cpp)

set(fm_api
	api/ppw_fat_model.h
	api/ppw_fat_types.h)

set(fm_tests
	code/fatmodel/fm_tests.cpp)

set(ipc_core
	code/ipc/ipc_msg.cpp
	code/ipc/ipc_shm.cpp)

set(ipc_api
	api/ppw_ipc_msg.h
	api/ppw_ipc_shm.h)

source_group(code FILES ${fm_core})
source_group(api FILES ${fm_api})
source_group("" FILES ${fm_tests})
source_group(code FILES ${ipc_core})
source_group(api FILES ${ipc_api})

add_library(fm_core STATIC ${fm_core} ${fm_api})
add_library(ppw::fm_core ALIAS fm_core)
target_include_directories(fm_core PUBLIC ${PPW_INC})
target_compile_definitions(fm_core PUBLIC ${PPW_CPP})

add_library(ipc_core STATIC ${ipc_core} ${ipc_api})
add_library(ppw::ipc_core ALIAS ipc_core)
target_include_directories(ipc_core PUBLIC ${PPW_INC})
target_compile_definitions(ipc_core PUBLIC ${PPW_CPP})

if (PPW_BUILD_TESTS)
add_executable(fm_tests ${fm_tests})
target_link_libraries(fm_tests PUBLIC fm_core)
if(MSVC)
	target_link_libraries(fm_tests PUBLIC "Ws2_32.lib")
endif()
endif()

set(PPW_PROJECTS fm_core ipc_core fm_tests)
set_target_properties(${PPW_PROJECTS} PROPERTIES FOLDER "ppw")


# MESSAGE( STATUS "PPW_SOURCE_DIR: " ${PPW_SOURCE_DIR} )
# MESSAGE( STATUS "CMAKE_BINARY_DIR:         " ${CMAKE_BINARY_DIR} )
# MESSAGE( STATUS "CMAKE_CURRENT_BINARY_DIR: " ${CMAKE_CURRENT_BINARY_DIR} )
# MESSAGE( STATUS "CMAKE_SOURCE_DIR:         " ${CMAKE_SOURCE_DIR} )
# MESSAGE( STATUS "CMAKE_CURRENT_SOURCE_DIR: " ${CMAKE_CURRENT_SOURCE_DIR} )
# MESSAGE( STATUS "PROJECT_BINARY_DIR: " ${PROJECT_BINARY_DIR} )
# MESSAGE( STATUS "PROJECT_SOURCE_DIR: " ${PROJECT_SOURCE_DIR} )
# MESSAGE( STATUS "EXECUTABLE_OUTPUT_PATH: " ${EXECUTABLE_OUTPUT_PATH} )
# MESSAGE( STATUS "LIBRARY_OUTPUT_PATH:     " ${LIBRARY_OUTPUT_PATH} )

