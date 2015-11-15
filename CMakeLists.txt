cmake_minimum_required(VERSION 2.6)
project(GSW_CPU)

# for youcompleteme.vim
set( CMAKE_EXPORT_COMPILE_COMMANDS 1 )

set(CMAKE_CXX_COMPILER c++)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -fopenmp")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -fopenmp")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

set(EIGEN_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/lib/eigen)
set(BLISS_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/lib/bliss)

set(CYMRIC_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/lib/cymric/include)
set(CYMRIC_LIBRARY ${PROJECT_SOURCE_DIR}/lib/cymric/lib/libcymric.a)

add_subdirectory("src" ".")

