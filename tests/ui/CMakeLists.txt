set(UI_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/qtests_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/logdata_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/logfiltereddata_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/crawlerwidget_test.cpp
)

if(NOT APPLE)
    LIST(APPEND UI_TEST_SOURCES  ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow_test.cpp)
endif()

add_executable(klogg_itests ${UI_TEST_SOURCES} ${BACKWARD_ENABLE})

target_link_libraries(klogg_itests klogg_ui klogg_utils Catch2 Qt6::Test test_utils)

if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    target_compile_definitions(klogg_itests PRIVATE CATCH_CONFIG_NO_COUNTER)
endif()

set_target_properties(klogg_itests PROPERTIES AUTOMOC ON)

add_backward(klogg_itests)

add_test(
    NAME klogg_itests
    COMMAND klogg_itests -platform offscreen
)
