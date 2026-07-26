# =============================================================================
# Tests
# =============================================================================
# Deliberately dependency-free: these are plain executables that return non-zero
# on failure, registered with CTest. Adding a test framework would pull a new
# dependency into a project that has none.

add_executable(rng_parity_test rng_parity_test.cpp)
target_include_directories(rng_parity_test PRIVATE ${CMAKE_SOURCE_DIR}/modules/core/inc)
target_compile_features(rng_parity_test PRIVATE cxx_std_20)

# lz/rng.h is header-only, so this links nothing from the project.
add_test(NAME rng_parity COMMAND rng_parity_test)
