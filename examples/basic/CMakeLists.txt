include_directories(${YAP_INTERNAL_INCLUDE_DIRECTORY})

add_executable(use_non_copyable_type use_non_copyable_type.cpp)
target_link_libraries(use_non_copyable_type ${USED_LIBS})
