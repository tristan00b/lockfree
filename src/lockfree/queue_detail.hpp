/****************************************************************************************\
  @file      queue_detail.hpp
  @package   lockfree
  @author    Tristan Bayfield
  @date      2020-03-20

  @copyright Copyright 2020, Tristan Bayfield.
\****************************************************************************************/

#ifndef LOCKFREE_QUEUE_DETAIL_HPP
#define LOCKFREE_QUEUE_DETAIL_HPP

#include <atomic>
#include <array>
#include <cstddef>
#include <chrono>
#include <future>
#include <initializer_list>
#include <iostream>
#include <optional>
#include <thread>
#include <type_traits>

#include "../type_traits.hpp"

/****************************************************************************************\

  Classes in this file combine to facilitate all the required functionality of the
  lock-free queue interface provided in `queue.hpp`. The use of `virual` has intentionally
  been avoided to reduce runtime overhead. The queue relies entirely on stack allocations
  so as to avoid heap allocations as well.

\****************************************************************************************/

namespace lockfree::detail {

template<typename data_type, std::size_t queue_size>
struct queue__base
{
public:
  static_assert(std::is_nothrow_default_constructible_v<data_type>);
  static_assert(std::is_nothrow_copy_constructible_v<data_type> ||
                std::is_nothrow_move_constructible_v<data_type>);

  using type = data_type; // simplifies implementation of data_write_policy_t

  template<typename U> // default behaviour is no-overwrite policy
  bool push(U && elem) noexcept
  {
    static_assert(std::is_convertible_v<std::remove_reference_t<U>, data_type>);

    auto old_write_index = write_index_.load();
    auto new_write_index = next_index(old_write_index);

    if (new_write_index == read_index_.load())
    { return false; } // the queue is full

    buffer_[old_write_index] = std::forward<U>(elem);
    write_index_.store(new_write_index);
    return true;
  }

  template<typename U>
  bool pop(U & elem) noexcept
  {
    static_assert(std::is_convertible_v<data_type, U>);

    auto old_read_index = read_index_.load();
    auto new_read_index = next_index(old_read_index);

    if (old_read_index == write_index_.load())
    { return false; } // queue is empty

    read_index_.store(new_read_index);
    elem = std::move(buffer_[old_read_index]);
    return true;
  }

protected:
  static constexpr auto next_index(std::size_t index) noexcept -> std::size_t
  { return ++index == buffer_size_ ? 0 : index; }

  static constexpr auto buffer_size_ = queue_size + 1;
  std::array<data_type, buffer_size_> buffer_ { };
  std::atomic<std::size_t> read_index_  { 0 }, write_index_ { 0 };
};

/** A lockfree SPSC queue implementation
 *
 * Provides thread-safe push and pop operations when used in an SPSC
 * (single-producer-single-consumer) configuration.
 *
 * The queue is implemented using a ring-buffer that is a statically allocated to be of
 * size `queue_size+1`, which provides a convenient means of deterimining the buffer's
 * empty and full states:
 *
 * `queue_is_full &nbsp;<-- read_index+1 == write_index`<br>
 * `queue_is_empty <-- read_index &nbsp;&nbsp;== write_index`
 *
 * Requires:
 * - `data_type` is no-throw default constructible
 * - `data_type` is no-throw copy constructible
 * - `data_type` is no-throw move constructible
 *
 * Note:
 * - Not all methods are thread-safe. Methods that are not thread-safe are documented as
 * such.
 *
 * Sources that helped me:
 * - Timur Doumler - C++ in the Audio Industry talks [CppCon, Juce]
 * - Anthony Williams - C++ Concurrency in Action [Manning]
 *
 * @tparam data_type The queue's internal storage type
 * @tparam queue_size The maximum number of elements that the queue can hold at any time
 * @tparam data_write_policy The policy governing the behaviour of push operations (except
 *   for those having the suffix `_wait`) when the queue is full
 */
template<typename data_type, std::size_t queue_size, typename Policy>
class queue : public Policy
{
public:
  static_assert(std::is_nothrow_default_constructible_v<data_type>);
  static_assert(std::is_nothrow_copy_constructible_v<data_type> ||
                std::is_nothrow_move_constructible_v<data_type>);

/****************************************************************************************\
  Object lifetime
\****************************************************************************************/

  /** The default constructor */
  queue() noexcept = default;

  /** Initializer list constructor
   *
   * Enables static initialization of the queue with up to `queue_size` elements provided
   * by an initializer list.
   *
   * @param[in] list An initializer list containing elements of type `data_type` to push
   *   to the queue
   */
  queue(std::initializer_list<data_type> list) noexcept
  {
    push_range(std::begin(list), std::end(list));
  }

/****************************************************************************************\
  Queue operations
\****************************************************************************************/

  /** Checks whether the queue is empty
   *
   * Returns a `bool` denoting whether the queue is empty or not.
   *
   * @returns `true if the queue is empty, `false` otherwise
   * @note not thread-safe
   */
  bool empty() const noexcept
  {
    return this->read_index_.load() == this->write_index_.load();
  }

  /** Checks whether the queue is full
   *
   * Returns a `bool` denoting whether the queue is full or not.
   *
   * @returns `true` if the queue is full, `false` otherwise
   */
  bool full() const noexcept
  {
    return this->next_index(this->write_index_.load()) == this->read_index_.load();
  }

  /** Returns the number of enqueued elements
   *
   * @note This operation is not thread-safe.
   *
   * @returns The number of elements in the queue
   */
  std::size_t size() const noexcept
  {
    return static_cast<std::size_t>(
      (this->buffer_size_ - this->read_index_.load() + this->write_index_.load()) % this->buffer_size_
    );
  }

  /** Discards the enqueued elements.
   *  @post The queue is empty.
   */
  void clear() noexcept
  {
    auto read_index = this->read_index_.load();
    // we use `atomic_compare_exchange_weak` because we don't care about spurious false
    // negatives, we're just going to loop anyway until the read_index it updated.
    while (! this->read_index_.compare_exchange_weak(read_index, this->write_index_.load()) );
  }

  /** Pushes an element to the queue
   *
   * Attempts to push an element to the queue, returning `true` when successful,
   * otherwise `false`. The exact behaviour depends upon the queue's `data_write_policy`.
   * Where the policy is set to `no_overwrite` (default), and the queue is full, the
   * operation will fail, and `false` is returned. Where the the policy is set to
   * `overwrite` the operation will never fail, simply writing over the next value when
   * space is needed.
   *
   * @tparam U Either `data_type` or a type that is convertible to `data_type`
   * @param[in] elem The data element to be placed on the queue
   * @returns `true` if the operation was successfull, `false` otherwise
   */
  template<typename U>
  bool push(U && elem) noexcept
  {
    return Policy::push( std::forward<U>(elem) );
  }

  /** Pushes an element to the queue, retrying until successful
   *
   * This method's runtime is unbounded as it repeatedly tries to add the element to the
   * queue until suceessful. There is no guarantee that this method will return, and its
   * use should be avoided unless it is explicitly known that the either the queue is not
   * full prior to pushing, and/or it was instantiated with the `overwrite` policy.
   * given as the third template parameter.
   *
   * @tparam U Either `data_type` or a type that is convertible to `data_type`
   * @param[in] elem The data element to be placed on the queue
   */
  template<typename U>
  void push_wait(U && elem) noexcept
  {
    while (! Policy::push(std::forward<U>(elem)) );
  }

  /** Adds an element to the queue, attempting `num_tries` times before failing
   *
   * Unlike `push_wait`, this method is guaranteed to return if not successful after
   * `num_tries` attempts.
   *
   * @tparam U Either `data_type` or a type that is convertible to `data_type`
   * @param[in] elem The data element to be placed on the queue
   * @param[in] num_tries The maximum number of attempts to be made before returning
   * @returns `true` if the operation was successfull, `false` otherwise
   */
  template<typename U>
  bool push_wait_for(U && elem, int num_tries) noexcept
  {
    for (auto i = 0; i<num_tries; ++i)
    { if (Policy::push( std::forward<U>(elem) )) return true; }

    return false;
  }

  /** Adds an element to the queue, successively retrying for the duration of `timeout`
   *
   * As with the `push_wait_for(U&&, int)` overload, this method is guaranteed to
   * eventyually return. Successive attempts to push to the queue are made until either
   * the operation is successful, or the time duration specified by `timeout` has fully
   * elapsed.
   *
   * @tparam U Either `data_type` or a type that is convertible to `data_type`
   * @tparam Rep A type representing the number of ticks (see STL documentation)
   * @tparam Period A ratio representing the tick period (see STL documentation)
   * @param[in] elem The data element to be placed on the queue
   * @param[in] timeout The max time to wait before returning
   * @returns `true` if the operation was successfull, `false` otherwise
   */
  template<typename U, typename Rep, typename Period>
  bool push_wait_for(U && elem, std::chrono::duration<Rep, Period> const& timeout) noexcept
  {
    auto fut = std::async([this] () {
      auto elem = data_type (/* default construct data_type */);
      push_wait(std::forward<U>(elem));
      return elem;
    });

    auto res = fut.wait_for(timeout);

    if (res == std::future_status::timeout)
    { return false; }

    elem = fut.get();

    return true;
  }

  /** Attempts to push a range of elements to the queue
   *
   * Attempts to push all the elements of the given range. If the queue fills up before
   * the entire range is pushed, then subsequent attempts to push new elements are
   * dependent on the queue's data write policy. If the `no_overwrite` policy has been
   * specified, then the operation stops. Otherwise, with each successive push the oldest
   * element will be overwritten until the operation completes.
   *
   * @tparam Iterator Any type that supports the features of an input iterator and which
   *   dereferences to an element of type `data_type`
   * @param[in] first An iterator marking the first element of the range to push
   * @param[in] last An iterator that abuts the last element of the range
   * @returns The number of elements that were pushed
   */
  template<typename Iterator>
  int push_range(Iterator first, Iterator last) noexcept
  {
    static_assert(std::is_nothrow_constructible_v<data_type, decltype(*first)>);

    int count = 0;
    while((first+count) != last)
    {
      if(! Policy::push(*(first+count))) break;
      ++count;
    }

    return count;
  }

  /** Attempts to push a range of elements to the queue
   *
   *  Similar to `push_range` but calls `push_wait` internally, and thus suffers from an
   *  an unbounded runtime.
   *
   *  @tparam Iterator Any type that supports the features of an input iterator and which
   *    dereferences to an element of type `data_type`
   *  @param[in] first An iterator marking the first element of the range to push
   *  @param[in] last An iterator demarking one element past the end of the range
   */
  template<typename Iterator>
  void push_range_wait(Iterator first, Iterator last) noexcept
  {
    static_assert(std::is_nothrow_constructible_v<data_type, decltype(*first)>);

    while(first != last)
    {
      push_wait(*first);
      ++first;
    }
  }

  /** Pops an element from the queue
   *
   * Removes the next element from the queue, assigning it to a given reference. When the
   * queue contains one or more elements the operation will complete successfully it will
   * save the popped value to the given reference and return `true`, otherwise it will
   * simply return `false`.
   *
   * @tparam U Either `data_type` or a type that is assignable from `data_type`
   * @param[out] elem The location to which the popped element will be assigned
   * @returns `true` if the pop was successful, `false` otherwise
   */
  template<typename U>
  bool pop(U & elem) noexcept
  {
    return Policy::pop(elem);
  }

  /** Convenience wrapper for queue::pop(data_type&)
   *
   *  @returns optional containing element if pop was successfull, std::nullopt otherwise
   */
  std::optional<data_type> pop() noexcept
  {
    auto elem = data_type (/* default construct data_type */);
    return Policy::pop(elem) ? std::optional<data_type> { std::move(elem) } : std::nullopt;
  }

  /** Pops an element from queue, retrying until successfull
   *
   * This method's runtime is unbounded, and will not return so long as the queue is
   * empty. An element must be available to be popped or else this method will try
   * indefinitely. If it cannot be guaranteed that the queue is not empty, or will not
   * remain empty indefinitely, either versio of `pop_wait_for` should be called instead.
   *
   * @tparam U Either `data_type` or a type that is assignable from `data_type`
   * @param[out] elem The location to which the popped element will be assigned
   */
  template<typename U>
  void pop_wait(U & elem) noexcept
  {
    // static_assert(std::is_nothrow_constructible_v<std::remove_reference_t<U>, data_type>);
    while (! Policy::pop(elem) );
  }

  /** Pops an element from the queue, retrying until successful
   *
   * The same as `pop_wait(U&) but takes no arguments and returns the element if
   * successful. This method's runtime is also unbounded.
   *
   * @returns The next element from the queue when the queue is not empty
   */
  data_type pop_wait() noexcept
  {
    auto elem = data_type (/* default construct data_type */);
    pop_wait(elem);
    return elem;
  }

  /** Pops an element element from the queue, retrying up to `num_tries` times
   *
   * Like, push_wait_for, tries to pop an element in upto `num_tries` attempts. This
   * method is also guaranteed to return, unlike `pop_wait`.
   *
   * @tparam U Either `data_type` or a type that is assignable from `data_type`
   * @param[out] elem The location to which the popped element will be assigned
   * @param num_tries The maximum number of attempts to make before returning
   * @returns `true` if the pop was successful, `false` otherwise
   */
  bool pop_wait_for(data_type & elem, int num_tries) noexcept
  {
    for (auto i = 0; i<num_tries; ++i)
    { if (Policy::pop(elem)) return true; }

    return false;
  }

  /** Pops an element from the queue, successively retrying for the duration of `timeout`
   *
   * As with `pop_wait_for(U&&, int)`, this method is guaranteed to eventyually return.
   * Successive attempts to pop from the queue are made until either the operation is
   * successful, or the time duration specified by `timeout` has fully elapsed.
   *
   * @tparam U Either `data_type` or a type that is assignable from `data_type`
   * @tparam Rep A type representing the number of ticks (see STL documentation)
   * @tparam Period a ratio representing the tick period (see STL documentation)
   * @param[out] elem The location to which the popped element will be assigned
   * @param[in] timeout The max time to wait before returning
   * @returns `true` if the operation was successful, `false` otherwise
   */
  template<typename U, typename Rep, typename Period>
  bool pop_wait_for(U & elem, std::chrono::duration<Rep, Period> const& timeout) noexcept
  {
    auto fut = std::async([this] () {
      auto elem = data_type (/* default construct data_type */);
      pop_wait(elem);
      return elem;
    });

    auto res = fut.wait_for(timeout);

    if (res == std::future_status::timeout)
    { return false; }

    elem = fut.get();

    return true;
  }

  /** Attempts to pops a range of elements from the queue
   *
   * Attempts to fill all the elements of a given range. If the queue empties before the
   * entire range is filled, then the method returns.
   *
   * @tparam Iterator Any type that meets the requirements for an input iterator and
   *   whose dereferenced type can be assigned by by an element of type `data_type`.
   * @param[out] first An iterator marking the first elementof the range to assign
   *   popped elements to
   * @param[out] last An iterator that abuts the last element of the range
   */
  template<typename Iterator>
  int pop_range(Iterator first, Iterator last) noexcept
  {
    static_assert(std::is_nothrow_assignable_v<decltype(*first), data_type>);

    int count = 0;
    while((first+count) != last)
    {
      if(! Policy::pop(*(first+count))) break;
      ++count;
    }

    return count;
  }

  /** Attempts to pop a range of elements from the queue
   *
   * Similar to `pop_range` but calls `pop_wait` internally, and this suffers from an
   * unbounded runtime.
   *
   * @tparam Iterator Any type that meets the requirements of `InputIterator`
   * @param[out] first An iterator marking the first element of a range to assign
   *   the sequence of popped elements to
   * @param[out] last An iterator demarking one element past the end of the range
   */
  template<typename Iterator>
  void pop_range_wait(Iterator first, Iterator last) noexcept
  {
    static_assert(std::is_nothrow_assignable_v<decltype(*first), data_type>);

    while(first != last)
    {
      pop_wait(*first);
      ++first;
    }
  }

  /** Pops an element from the queue then applies a given function to it
   *
   * The provided function may optionally be called with a list of arguments that are
   * supplied to it following the popped element.
   *
   * @tparam Fn Function's type
   * @tparam Args The sequence of function argument types, less `data_type`
   * @param fn The function to apply
   * @param args The functions arguments
   */
  template <typename Fn, typename... Args>
  bool consume_with(Fn && fn, Args &&... args)
  {
    auto elem = data_type (/* default construct data_type */);
    if (Policy::pop(&elem))
    {
      std::invoke(std::forward<Fn>(fn), std::move(elem), std::forward<Args>(args)...);
      return true;
    }

    return false;
  }

}; // class queue


template<typename Base, enum data_write_policy>
class data_write_policy_t;


template<typename Base>
class data_write_policy_t<Base, data_write_policy::overwrite> : public Base
{
public:

  /**
   *  When the queue is full, the oldest element is overwritten.
   *
   *  Invariant:
   *  - When the queue is full `write_index` is `n` and `read_index` is `n+1 % buffer_size`
   *    where `read_index` is pointing to the oldest element about to be overwritten.
   *
   *  To overwrite the oldest element, `read_index` must be incremented to point the next
   *  oldest element prior to writing the next element and incrementing `write_index`.
   */
  template<typename U>
  bool push(U && elem) noexcept
  {
    static_assert(std::is_convertible_v<std::remove_cvref_t<U>, typename Base::type>);

    auto old_read_index = this->read_index_.load();
    auto new_read_index = next_index(old_read_index);
    // CAS because we are on the write thread
    while(! this->read_index_.compare_exchange_weak(old_read_index, new_read_index) );
    // The old read index is the new write index
    this->buffer_[old_read_index] = std::forward<U>(elem);
    this->write_index_.store(old_read_index);
    // Although this method will never fail, we return true to be consisten with the API
    return true;
  }
};


template<typename Base>
class data_write_policy_t<Base, data_write_policy::no_overwrite> : public Base
{
public:
  template<typename U>
  bool push(U && elem) noexcept
  {
    return Base::push( std::forward<U>(elem) );
  }
};

/****************************************************************************************\
  Queue Partial Template Specializations

  Inheritance Order:

  `queue --> data_write_policy_t --> queue__base`

  where both `queue` and `data_write_policy_t` depend on the behaviours of `queue__base`,
  and `queue` further dependes on the policy-specific behaviours of `data_write_policy_y`.
  This configuration permits us to avoid using `virtual`.
\****************************************************************************************/

template<typename type, std::size_t size>
using queue__overwrite_policy_t  =
  class queue<type, size, data_write_policy_t<queue__base<type, size>, data_write_policy::overwrite>>;

template<typename type, std::size_t size>
using queue__no_overwrite_policy_t =
  class queue<type, size, data_write_policy_t<queue__base<type, size>, data_write_policy::no_overwrite>>;

} // namespace lockfree::detail

#endif // LOCKFREE_QUEUE_DETAIL_HPP
