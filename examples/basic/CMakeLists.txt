include_directories(${YAP_INTERNAL_INCLUDE_DIRECTORY})

add_executable(use_non_copyable_type use_non_copyable_type.cpp)
target_link_libraries(use_non_copyable_type ${USED_LIBS})

add_executable(nthreaded_stages nthreaded_stages.cpp)
target_link_libraries(nthreaded_stages ${USED_LIBS})

add_executable(use_filtered use_filtered.cpp)
target_link_libraries(use_filtered ${USED_LIBS})

add_executable(use_hatched use_hatched.cpp)
target_link_libraries(use_hatched ${USED_LIBS})
