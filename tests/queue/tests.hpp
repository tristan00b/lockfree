/****************************************************************************************\
  File:    tests.hpp
  Package: lockfree/tests/queue
  Created: 2020-03-30 by Tristan Bayfield

  Copyright 2020, Tristan Bayfield.
\****************************************************************************************/

#ifndef LOCKFREE_QUEUE_TESTS_HPP
#define LOCKFREE_QUEUE_TESTS_HPP


template<typename T>
class intl_t_ {
public:
  intl_t_ (void) = default;
  intl_t_ (int val) noexcept : value_ { val } {}

  // template<typename U>
  // intl_t_ (U const& other) : value_(other.value_) { }

  T value_;
};


template<typename T>
class extl_t_ {
public:
  extl_t_ () : value_ { 0 }  { }
  extl_t_ (T val) : value_ { val } { }

  template<typename U>
  extl_t_ (U const& other) : value_(other.value_) { }

  template<typename U>
  operator U (void) const noexcept { return U { value_ }; }

  template<typename U>
  bool operator==(U const& other) const noexcept { return value_ == other.value_; }

  T value_;
};

using intl_t = intl_t_<int>;
using extl_t = extl_t_<int>;

#endif