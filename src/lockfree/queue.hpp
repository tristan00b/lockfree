/****************************************************************************************\
  @file      queue.hpp
  @package   lockfree
  @author    Tristan Bayfield
  @date      2020-04-04

  @copyright Copyright 2020, Tristan Bayfield.
\****************************************************************************************/

#ifndef LOCKFREE_QUEUE_HPP
#define LOCKFREE_QUEUE_HPP

#include <cstddef>
#include <initializer_list>
#include <optional>
#include <type_traits>

namespace lockfree {

/** Polices for handling push operations when the queue is full
 *
 * The policy chosen for a given queue instantiation determines the behaviour of push
 * operations (except for those having the `_wait` suffix) that are requested whenever
 * the queue is full. The default is to `no_overwrite` if no policy is specified.
 */
enum class data_write_policy
{
  /** While the queue is full, no new elements can be pushed to to it. */
  no_overwrite,
  /** While the queue is full, any new element that is pushed will overwrite the oldest element. */
  overwrite
};

} // namespace lockfree

#include "queue_detail.hpp"

namespace lockfree {

/**
 * @copydoc lockfree::detail::queue
 * @extends lockfree::detail::queue
 */
template<typename data_type,
         std::size_t queue_size,
         enum data_write_policy = data_write_policy::no_overwrite>
class queue
{
};

template<typename type, std::size_t size>
class queue<type, size, data_write_policy::overwrite>
: public detail::queue__overwrite_policy_t<type, size>
{
  using super = detail::queue__overwrite_policy_t<type, size>;
  using super::super; // inherit superclass constructors;
};


template<typename type, std::size_t size>
class queue<type, size, data_write_policy::no_overwrite>
: public detail::queue__no_overwrite_policy_t<type, size>
{
  using super = detail::queue__no_overwrite_policy_t<type, size>;
  using super::super; // inherit superclass constructors;
};

} // namespace lockfree

#endif // LOCKFREE_QUEUE_HPP
