include_directories(${YAP_INTERNAL_INCLUDE_DIRECTORY})

add_executable(two_dimensional_transform two_dimensional_transform.cpp)
target_link_libraries(two_dimensional_transform ${USED_LIBS})
