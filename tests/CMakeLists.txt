# Compile the entrypoint with a test-only name so GoogleTest can supply main.
add_library(stay_awake_test_entrypoint OBJECT "${PROJECT_SOURCE_DIR}/src/main.cpp")
target_compile_definitions(stay_awake_test_entrypoint PRIVATE main=StayAwakeMain)
target_link_libraries(stay_awake_test_entrypoint PRIVATE StayAwake::core)
set_target_properties(stay_awake_test_entrypoint PROPERTIES CXX_EXTENSIONS OFF)
stay_awake_enable_warnings(stay_awake_test_entrypoint)

add_executable(
    stay_awake_tests
    main.test.cpp
    application/application.test.cpp
    application/awake_controller.test.cpp
    application/close_controller.test.cpp
    application/durations.test.cpp
    application/window_selection.test.cpp
)
target_compile_features(stay_awake_tests PRIVATE cxx_std_17)
target_link_libraries(stay_awake_tests PRIVATE stay_awake_test_entrypoint StayAwake::core GTest::gtest_main)
set_target_properties(stay_awake_tests PROPERTIES CXX_EXTENSIONS OFF)
stay_awake_enable_warnings(stay_awake_tests)

if(MINGW)
    target_link_options(stay_awake_tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()

include(GoogleTest)
gtest_discover_tests(stay_awake_tests DISCOVERY_MODE PRE_TEST)
