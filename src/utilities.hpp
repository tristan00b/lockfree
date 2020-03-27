/****************************************************************************************\
  File:    utilities.hpp
  Package: lockfree
  Created: 2020-04-02 by Tristan Bayfield

  Copyright 2020, Tristan Bayfield.
\****************************************************************************************/

#include <type_traits>

#if (defined(__GNUG__ ) && (__cplusplus < 201709L)) || \
    (defined(__clang__) && (__cplusplus < 202002L)) || \
    (defined(_MSC_VER ) && (_MSC_VER    < 1924   ))

namespace std {

template<typename T>
struct remove_cvref
{
  using type = std::remove_cv_t<std::remove_reference_t<T>>;
};

template<typename T>
using remove_cvref_t = typename remove_cvref<T>::type;

} // namespace std

#endif
