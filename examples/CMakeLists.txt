include_directories(${YAP_INTERNAL_HEADERS})

add_executable(basic basic.cpp)
target_link_libraries(basic ${USED_LIBS})
