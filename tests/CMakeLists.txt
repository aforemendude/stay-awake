add_executable(
    stay_awake_tests
    application/application.test.cpp
    application/awake_controller.test.cpp
    application/close_controller.test.cpp
    application/durations.test.cpp
    application/window_selection.test.cpp
)
target_compile_features(stay_awake_tests PRIVATE cxx_std_17)
target_link_libraries(stay_awake_tests PRIVATE StayAwake::core GTest::gtest_main)
set_target_properties(stay_awake_tests PROPERTIES CXX_EXTENSIONS OFF)
stay_awake_enable_warnings(stay_awake_tests)

if(MINGW)
    target_link_options(stay_awake_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()

include(GoogleTest)
gtest_discover_tests(stay_awake_tests DISCOVERY_MODE PRE_TEST)
