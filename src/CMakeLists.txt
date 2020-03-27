#
# Lockfree Library
#

add_library(lockfree INTERFACE)

target_sources(lockfree
  INTERFACE
    lockfree/queue.hpp
  )

target_include_directories(lockfree
  INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}"
  )
