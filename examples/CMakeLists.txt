include_directories(${YAP_INTERNAL_INCLUDE_DIRECTORY})

add_executable(basic basic.cpp)
target_link_libraries(basic ${USED_LIBS})
