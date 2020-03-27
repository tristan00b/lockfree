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

#include "../utilities.hpp"

namespace lockfree::detail {


struct data_write_policy
{
public:

  class no_overwrite
  {

  };

  class overwite
  {
  };

};


template <typename T,
          std::size_t queue_size>
class queue {

  static_assert(std::is_default_constructible_v<T>);
  static_assert(std::is_copy_constructible_v<T> || std::is_move_constructible_v<T>);

/****************************************************************************************\
  Lifetime management
\****************************************************************************************/

public:

  queue() noexcept = default;

  queue(std::initializer_list<T> l) noexcept
  {
    push_range_weak(std::begin(l), std::end(l));
  }


/****************************************************************************************\
  Queue operations
\****************************************************************************************/

  bool empty() const noexcept
  {
    return read_index_.load() == write_index_.load();
  }


  bool full() const noexcept
  {
    return next_index(write_index_.load()) == read_index_.load();
  }


  std::size_t size() const noexcept
  {
    return static_cast<std::size_t>(
      (buffer_size_ - read_index_.load() + write_index_.load()) % buffer_size_
    );
  }


  void clear() noexcept
  {
    auto read_index = read_index_.load();
    // we use `atomic_compare_exchange_weak` because we don't care about spurious false
    // negatives, we're just going to loop anyway until the read_index it updated.
    while (! read_index_.compare_exchange_weak(read_index, write_index_.load()) );
  }


  template <typename U>
  bool push(U && elem) noexcept
  {
    static_assert(std::is_convertible_v<std::remove_cvref_t<U>, T>);

    auto old_write_index = write_index_.load();
    auto new_write_index = next_index(old_write_index);

    if (new_write_index == read_index_.load())
    { return false; } // the queue is full

    buffer_[old_write_index] = std::forward<U>(elem);
    write_index_.store(new_write_index);

    return true;
  }


  bool pop(T & elem) noexcept
  {
    auto old_read_index = read_index_.load();
    auto new_read_index = next_index(old_read_index);

    if (old_read_index == write_index_.load())
    { return false; } // queue is empty

    read_index_.store(new_read_index);
    elem = std::move(buffer_[old_read_index]);

    return true;
  }


  std::optional<T> pop() noexcept
  {
    auto elem = T { };
    return pop(elem) ? std::optional<T> { std::move(elem) } : std::nullopt;
  }


  template<typename U>
  void push_wait(U && elem) noexcept
  {
    while (! push(std::forward<U>(elem)) );
  }


  template<typename U>
  void pop_wait(U & elem) noexcept
  {
    // static_assert(std::is_nothrow_constructible_v<std::remove_reference_t<U>, T>);
    while (! pop(elem) );
  }


  T pop_wait() noexcept
  {
    auto elem = T (/* default construct T */);
    pop_wait(elem);
    return elem;
  }


  template<typename U, typename Duration>
  bool push_wait_for(U & elem, Duration dur) noexcept;


  template<typename U>
  bool push_wait_for(U && elem, int num_tries) noexcept
  {
    for (auto i = 0; i<num_tries; ++i)
    { if (push( std::forward<U>(elem) )) return true; }

    return false;
  }


  template<typename U, typename Rep, typename Period>
  bool push_wait_for(U && elem, std::chrono::duration<Rep, Period> const& timeout_duration) noexcept
  {
    auto fut = std::async([this] () {
      auto elem = T (/* default construct T */);
      push_wait(std::forward<U>(elem));
      return elem;
    });

    auto res = fut.wait_for(timeout_duration);

    if (res == std::future_status::timeout)
    { return false; }

    elem = fut.get();

    return true;
  }


  template<typename Duration>
  bool pop_wait_for(T & elem, Duration dur) noexcept;


  bool pop_wait_for(T & elem, int num_tries) noexcept
  {
    for (auto i = 0; i<num_tries; ++i)
    { if (pop(elem)) return true; }

    return false;
  }


  template<typename Rep, typename Period>
  bool pop_wait_for(T & elem, std::chrono::duration<Rep, Period> const& timeout_duration) noexcept
  {
    auto fut = std::async([this] () {
      auto elem = T();
      pop_wait(elem);
      return elem;
    });

    auto res = fut.wait_for(timeout_duration);

    if (res == std::future_status::timeout)
    { return false; }

    elem = fut.get();

    return true;
  }


  template<typename Iterator>
  void push_range_strong(Iterator first, Iterator last) noexcept
  {
    static_assert(std::is_nothrow_constructible_v<decltype(*first), T>);
    while(first != last)
    {
      push_wait(*first);
      ++first;
    }
  }


  template<typename Iterator>
  void push_range_weak(Iterator first, Iterator last) noexcept
  {
    static_assert(std::is_nothrow_constructible_v<decltype(*first), T>);
    while(first != last)
    {
      push(*first);
      ++first;
    }
  }


  template<typename Iterator>
  void pop_range(Iterator first, Iterator last) noexcept
  {
    //static_assert(std::is_convertible_v<decltype(*first), T>);

    while(first != last)
    {
      pop_wait(*first);
      ++first;
    }
  }


  template <typename Fn, typename... Args>
  bool consume_with(Fn && fn, Args &&... args)
  {
    auto elem = T { };
    if (pop(&elem))
    {
      std::invoke(std::forward<Fn>(fn), std::move(elem), std::forward<Args>(args)...);
      return true;
    }

    return false;
  }


private:

  static constexpr auto next_index(std::size_t index) noexcept -> std::size_t
  {
    return ++index == buffer_size_ ? 0 : index;
  }


  static constexpr auto buffer_size_ = queue_size + 1;

  std::array<T, buffer_size_> buffer_ { };

  std::atomic<std::size_t> read_index_  { 0 }, write_index_ { 0 };
};

} // namespace lockfree::detail

#endif /* LOCKFREE_QUEUE_DETAIL_HPP */
