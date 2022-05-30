set(TARGET_NAME cpp_calculator)

add_executable (${TARGET_NAME} calc.cpp main.cpp)

include_directories(
    "${PROJECT_SOURCE_DIR}/include"
)