# Add test cpp file
add_executable(klogg_tests
    linepositionarray_test.cpp
    patternmatcher_test.cpp
    textencoding_test.cpp
    theme_test.cpp
    tests_main.cpp
)

target_link_libraries(klogg_tests klogg_ui klogg_utils klogg_logging Catch2 Qt6::Test)

if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    target_compile_definitions(klogg_tests PRIVATE CATCH_CONFIG_NO_COUNTER)
endif()

set_target_properties(klogg_tests PROPERTIES AUTOMOC ON)

add_test(
    NAME klogg_tests
    COMMAND klogg_tests -platform offscreen
)
