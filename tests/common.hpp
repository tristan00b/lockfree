/****************************************************************************************\
  File:    common.hpp
  Package: lockfree/tests
  Created: 2020-04-01 by Tristan Bayfield

  Copyright 2020, Tristan Bayfield.
\****************************************************************************************/

#ifndef LOCKFREE_TESTS_COMMON_HPP
#define LOCKFREE_TESTS_COMMON_HPP


#include <chrono>
using namespace std::chrono_literals;


#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch2/catch.hpp>


#include "helpers.hpp"


#endif /* LOCKFREE_TESTS_COMMON_HPP */