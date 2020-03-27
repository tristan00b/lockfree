#
# Lockfree Tests
#

add_executable(tests)

add_dependencies(tests lockfree)

target_sources(tests
  PUBLIC
    helpers.hpp
    tests.cpp
    common.hpp

    queue/tests.cpp
    queue/tests.hpp
  )

target_link_libraries(tests
  lockfree
  Catch2::Catch2
  )

# Automatically register Catch2 tests with CTest
catch_discover_tests(tests)