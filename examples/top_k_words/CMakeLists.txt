include_directories(${YAP_INTERNAL_INCLUDE_DIRECTORY})

add_executable(top_k_words top_k_words.cpp)
target_link_libraries(top_k_words ${USED_LIBS})
