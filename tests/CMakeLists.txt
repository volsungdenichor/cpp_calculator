include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/e2239ee6043f73722e7aa812a459f54a28552929.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(cpp_calculator_tests parser.cpp "${PROJECT_SOURCE_DIR}/src/calc.cpp")
include_directories(
    "${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(cpp_calculator_tests gtest_main gmock_main)
add_test(NAME cpp_calculator_tests COMMAND cpp_calculator_tests)