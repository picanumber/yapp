set(CMAKE_PROJECT_VERSION "1")
set(CMAKE_PROJECT_VERSION_MAJOR "1")
set(CMAKE_PROJECT_VERSION_MINOR "1")
set(CMAKE_PROJECT_VERSION_PATCH "1")
set(CMAKE_VERSION_STRING "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH}")

cmake_policy(SET CMP0048 NEW)
project(yet-another-pipeline VERSION "${LIB_VERSION_STRING}" LANGUAGES CXX)
 
cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20) 
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake/finders)

set(YAP_INTERNAL_INCLUDE_DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(THREADS_PREFER_PTHREAD_FLAG ON)                                                                                  
find_package(Threads REQUIRED)

SET(USED_LIBS
    Threads::Threads
)

if (WIN32)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

add_subdirectory(examples)
add_subdirectory(test)

#function(cat IN_FILE OUT_FILE)
    #message("printing...${IN_FILE}")
  #file(READ ${IN_FILE} CONTENTS)
  #file(APPEND ${OUT_FILE} "${CONTENTS}")
#endfunction()

# Prepare a temporary file to "cat" to:
#file(WRITE somefile.sql.in "")

## Call the "cat" function for each input file
#foreach(PACKAGE_SQL_FILE ${YAP_INTERNAL_HEADERS}/yap/)
#  cat(${PACKAGE_SQL_FILE} somefile.sql.in)
#endforeach()
#
## Copy the temporary file to the final location
#configure_file(somefile.sql.in somefile.sql COPYONLY)
#
#file(REMOVE somefile.sql.in)

