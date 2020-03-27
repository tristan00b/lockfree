/****************************************************************************************\
  File:    helpers.hpp
  Package: lockfree/tests
  Created: 2020-03-22 by Tristan Bayfield

  Copyright 2020, Tristan Bayfield.
\****************************************************************************************/

#ifndef LOCKFREE_TESTS_HELPERS_HPP
#define LOCKFREE_TESTS_HELPERS_HPP


#include <array>
#include <numeric>


namespace lockfree::tests::helpers {

template <typename data_type, std::size_t   array_size>
auto iota() -> std::array<data_type, array_size>
{
  auto d = std::array<data_type, array_size> { };
  std::iota(std::begin(d), std::end(d), 0);
  return d;
}

};


#endif /* LOCKFREE_TESTS_HELPERS_HPP */
