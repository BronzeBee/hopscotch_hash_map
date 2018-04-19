cmake_minimum_required(VERSION 3.9)
project(hopscotch_hash_map LANGUAGES C VERSION 0.1.0)

set(PROJECT_TEST ${PROJECT_NAME}_test)

include_directories(include)

# Main library target
add_library(
        ${PROJECT_NAME}
        include/hs_hash_map/hs_hash_map.h
        src/hs_hash_map.c
)

target_compile_features(${PROJECT_NAME} PUBLIC c_std_11)

set_target_properties(${PROJECT_NAME} PROPERTIES C_EXTENSIONS OFF)

configure_file(test/string_keys.txt string_keys.txt COPYONLY)
configure_file(test/string_values.txt string_values.txt COPYONLY)

# Test target
add_executable(${PROJECT_TEST} test/test.c)

set_target_properties(${PROJECT_TEST} PROPERTIES C_EXTENSIONS OFF)

target_link_libraries(${PROJECT_TEST} ${PROJECT_NAME})

include(CTest)
if(BUILD_TESTING)
    add_test(NAME test_hopscotch_hash_map COMMAND ${PROJECT_TEST})
endif()


