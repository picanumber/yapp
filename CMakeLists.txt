project(yet-another-pipeline)
 
cmake_minimum_required(VERSION 3.16)

set (CMAKE_CXX_STANDARD 20) 
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake/finders)

set(YAP_INTERNAL_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(THREADS_PREFER_PTHREAD_FLAG ON)                                                                                  
find_package(Threads REQUIRED)

SET(USED_LIBS
    Threads::Threads
)

#SET(YAP_SOURCES
#    source/RemoteMemory.cpp
#    source/LoopbackMemory.cpp
#    source/RemUtilities.cpp
#    source/DsCommon.cpp
#)

if (WIN32)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

add_subdirectory(examples)
add_subdirectory(test)

