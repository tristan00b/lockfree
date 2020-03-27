/****************************************************************************************\
  @file      queue.hpp
  @package   lockfree
  @author    Tristan Bayfield
  @date      2020-04-04

  @copyright Copyright 2020, Tristan Bayfield.
\****************************************************************************************/

#ifndef LOCKFREE_QUEUE_HPP
#define LOCKFREE_QUEUE_HPP

#include "queue_detail.hpp"
#include <experimental/propagate_const>
#include <memory>

namespace lockfree {

/** A Lockfree SPSC queue implementation
 *
 *  Implemented using a ring-buffer, where the push and pop operations are thread-safe
 *  when the queue is used in a single-producer-single-consumer context.
 *
 *  The internal ring-buffer is a staticaly allocated array of size queue_size+1.
 *  This provides a convenient way to determine the queue's empty and full states
 *  respectively:
 *
 *  queue_is_empty <-- read_index   == write_index
 *  queue_is_full  <-- read_index+1 == write_index
 *
 *  Requires:
 *  - T is default constructible
 *  - T is copy constructible || move constructible
 *  Recommended:
 *  - T is trivially constructible
 *
 *  Sources that helped me greatly:
 *  - Timur Doumler - C++ in the Audio Industry talks [CppCon, Juce]
 *  - Anthony Williams - C++ Concurrency in Action [Manning]
 */
template<typename data_type, std::size_t queue_size>
class queue;


/** Polices for handling push requests when the queue is full.
 */
enum class data_write_policy
{
  /** When full, no new elements can by pushed to the queue.
   */
  no_overwrite,

  /** When full, a new element that is pushed to the queue overwrites the oldest element.
   */
  overwrite
};


template <typename data_type,
          std::size_t queue_size>
class queue {

  static_assert(std::is_default_constructible_v<data_type>);
  static_assert(std::is_copy_constructible_v<data_type> || std::is_move_constructible_v<data_type>);

/****************************************************************************************\
  Lifetime managment
\****************************************************************************************/

public:

  queue() noexcept : queue_ ( std::make_unique<detail::queue<data_type, queue_size>>() )
  { }

  /** Initializer list constructor
   *  Enables static initialization of upto `queue_size` elements
   */
  queue(std::initializer_list<data_type> l) noexcept
  : queue_ ( std::make_unique<detail::queue<data_type, queue_size>>(std::move(l)) )
  { }


/****************************************************************************************\
  Queue operations
\****************************************************************************************/

  /** Checks whether the queue is empty.
   *
   *  @returns `true if the queue is empty, `false` otherwise
   *  @note not thread-safe
   */
  inline bool empty() const noexcept { return queue_->empty(); }


  /** Checks whether the queue is full.
   *
   *  @return `true` if the queue is full, `false` otherwise
   *  @note not thread-safe
   */
  inline bool full() const noexcept { return queue_->full(); }


  /** Returns the number of enqueued elements.
   *
   *  @note not thread-safe.
   */
  inline std::size_t size() const noexcept { return queue_->size(); }


  /** Discards the enqueued elements.
   *
   *  @post The queue is empty.
   */
  inline void clear() noexcept { queue_->clear(); }


  /** Adds an element to the queueu.
   *
   *  @param[in] elem The data element to be placed on the queue.
   *  @return `true` if the push was successfull, `false` otherwise.
   */
  template <typename U>
  inline bool push(U && elem) noexcept { return queue_->push( std::forward<U>(elem) ); }


  /** Removes and element from the queue.
   *
   *  @param[in,out] elem Where the popped element will be saved.
   *  @return true if the pop was successful, false otherwise.
   *  @note Type U must be constructible from data_type.
   */
  inline bool pop(data_type & elem) noexcept { return queue_->pop(elem); }


  /**
   *
   */
  inline std::optional<data_type> pop() noexcept { return queue_->pop(); }


  /**
   *
   */
  template<typename Iterator>
  inline void pop_range(Iterator first, Iterator last) noexcept
  { queue_->pop_range(first, last); }



  /**
   *
   */
  template<typename U>
  inline void pop_wait(U & elem) noexcept { queue_->pop_wait(elem); }


  /**
   *
   */
  inline data_type pop_wait() noexcept { return queue_->pop_wait(); }


  /**
   *
   */
  template<typename R, typename P>
  inline bool pop_wait_for(data_type & elem, std::chrono::duration<R, P> const& timeout) noexcept
  { return queue_->pop_wait_for(elem, timeout); }


  /**
   *
   */
  inline bool pop_wait_for(data_type & elem, int try_count) noexcept
  { return queue_->pop_wait_for(elem, try_count); }


  /**
   *
   */
  template<typename U>
  inline void push_wait(U && elem) noexcept { queue_->push_wait( std::forward<U>(elem) ); }


  /**
   *
   */
  template<typename U, typename R, typename P>
  inline bool push_wait_for(U && elem, std::chrono::duration<R, P> const& timeout) noexcept
  { return queue_->push_wait_for( std::forward<U>(elem), timeout ); }


  /**
   *
   */
  template<typename Iterator>
  inline void push_range(Iterator first, Iterator last) noexcept
  { queue_->push_range(first, last); }


  /** Consumes one element by applying a function to it.
   *
   *  The function fn must take a queue element as its first argument, which is supplied
   *  as a result of a pop operation. Further arguments to f may be supplied by the caller
   *  in addition to fn itself.
   */
  template <typename F, typename... Args>
  inline bool consume_with(F && func, Args &&... args)
  { return queue_->consume_with( std::forward<F>(func), std::forward<Args>(args)... ); }


private:

  std::experimental::propagate_const<
    std::unique_ptr<detail::queue<data_type, queue_size>>
  > queue_;

};

} // namespace lockfree

#endif /* LOCKFREE_QUEUE_HPP */
