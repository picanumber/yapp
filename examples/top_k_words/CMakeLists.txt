include_directories(${YAP_INTERNAL_INCLUDE_DIRECTORY})

set(CMAKE_CXX_STANDARD 20)

add_executable(top_k_words top_k_words.cpp)
target_link_libraries(top_k_words ${USED_LIBS})
