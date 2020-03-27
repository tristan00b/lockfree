/****************************************************************************************\
  File:    tests.cpp
  Package: lockfree/tests/queue
  Created: 2020-03-29 by Tristan Bayfield

  Copyright 2020, Tristan Bayfield.
\****************************************************************************************/

#include "tests.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <thread>
#include <utility>
#include <lockfree/queue.hpp>

#include "../common.hpp"


using namespace lockfree;
using namespace std::chrono_literals;


using data_type = int;
constexpr auto queue_size = std::size_t { 25 };


/****************************************************************************************\
  Single-threaded tests
\****************************************************************************************/

TEST_CASE("Can construct from initializer list", "[queue]")
{
  auto q = queue<data_type, queue_size> {
    data_type { },
    data_type { },
    data_type { },
    data_type { }
  };
}

TEST_CASE("Can push and pop elements", "[queue]")
{
  auto q = queue<data_type, queue_size> { };
  auto res = data_type { };

  // Operations _must_ always succeed in single-threaded contexts
  REQUIRE(q.push(42));
  REQUIRE(q.pop(res));
  REQUIRE(res == 42);
}

TEST_CASE("Popped elements are the same as pushed", "[queue]")
{
  auto input_buffer = tests::helpers::iota<data_type, queue_size>();
  auto output_buffer = std::array<data_type, queue_size>();
  auto q = queue<data_type, queue_size> { };

  // Push the test data onto the queue
  for (auto elem : input_buffer)
  { q.push(std::move(elem)); }

  // The queue is now expected to be full
  CHECK_FALSE(q.push(data_type { }));

  // Pop the test data off the queue
  for (auto & elem : output_buffer)
  { q.pop(elem); }

  // The queue is now expected to be empty
  CHECK_FALSE(q.pop());

  // The output data is expected to equal the original input data
  REQUIRE(output_buffer == input_buffer);
}


TEST_CASE("Queue correctly reports when it is empty and full", "[queue]")
{
  auto input_buffer = tests::helpers::iota<data_type, queue_size>();
  auto output_buffer = std::array<data_type, queue_size>();
  auto q = queue<data_type, queue_size> { };

  // The queue is expected to be empty before pushing any elements to it
  REQUIRE(q.empty());

  // Pushthe test data onto the queue
  for (auto elem : input_buffer)
  { q.push(std::move(elem)); }

  // The queue is now expected to be full
  REQUIRE(q.full());

  // Pop the test data off the queue
  for (auto & elem : input_buffer)
  { q.pop(elem); }

  // The queue is now expected to be empty
  REQUIRE(q.empty());
}


TEST_CASE("Queue reports the correct number of enqueued elements", "[queue]")
{
  constexpr auto data_size = queue_size*2;

  auto q = queue<data_type, queue_size> { };

  for (auto i = 0; i<data_size; ++i)
  {
    // The queue is expected to have up to a maximum of queue_size elements
    CHECK(q.size() == std::min(static_cast<std::size_t>(i), queue_size));
    q.push(i);
  }
}

TEST_CASE("Can clear the queue", "[queue]")
{
  auto q = queue<data_type, queue_size> {
    data_type { },
    data_type { },
    data_type { },
    data_type { }
  };

  REQUIRE_FALSE(q.empty());

  q.clear();

  REQUIRE(q.empty());
}


TEST_CASE("Queue can be instantiated from an initializer list", "[queue]")
{
  auto q = queue<int, queue_size> { 0, 1, 2, 3, 4 };
  auto expected = { 0, 1, 2, 3, 4 };

  for (auto const& elem : expected)
  { REQUIRE(q.pop().value_or(-1) == elem); }
}


// // TODO
// TEST_CASE("Queue properly handles type conversions", "[queue]")
// {
//   // Part a. Test with trivial types
//   auto q1 = queue<int, queue_size> { };

//   REQUIRE(q1.push(std::size_t { 1 }));
//   REQUIRE(q1.pop().value_or(-1) == 1);

//   auto q2 = queue<intl_t, queue_size> { };
//   auto ivalue = extl_t {  4 };
//   auto ovalue = extl_t { -1 };

//   REQUIRE(q2.push(ivalue));
//   REQUIRE(q2.pop(ovalue));
//   REQUIRE((ovalue == ivalue)); // Catch balks w/o extra parens
// }



// TEST_CASE("Queue accepts move-only types", "[queue]")
// {
//   auto q = queue<std::unique_ptr<int>, queue_size> { };

//   REQUIRE(q.push( std::make_unique<int>(42) ));
//   REQUIRE(q.pop().has_value());
// }



/****************************************************************************************\
  Multi-threaded tests
\****************************************************************************************/



// TEST_CASE("Can be used safely in a multithreaded context", "[queue, multi-threaded]")
// {
//   /* Note: passing this test is not a guarantee of thread-safety! */

//   constexpr auto data_size = 100u;

//   auto input_buffer = tests::helpers::iota<data_type, data_size>();
//   auto output_buffer = std::array<data_type, data_size> { };
//   auto q = queue<data_type, queue_size> { };

//   // Asynchronously pushes each element of input_buffer to the queue
//   auto producer = std::thread([&q, &input_buffer] () {
//     for (auto elem : input_buffer)
//       q.push_wait( std::move(elem) );
//   });

//   // Asynchronously pops each element of the queue and assigns it to the corresponding
//   // element of output_buffer.
//   auto consumer = std::thread([&q, &output_buffer] () {
//       for (auto & elem : output_buffer)
//         q.pop_wait(elem);
//   });

//   producer.join();
//   consumer.join();

//   REQUIRE(output_buffer == input_buffer);
// }



// TEST_CASE("Can time out on *_wait_for operations", "[queue, multi-threaded]")
// {
//   constexpr auto data_size = 100u;

//   // With a timeout of 1 nanosecond, we are pretty much guaranteed to have timeouts
//   auto timeout = 1ns;
//   auto push_timeouts = std::array<bool, data_size> { };
//   auto pop_timeouts  = std::array<bool, data_size> { };

//   auto q = queue<data_type, queue_size> { };

//   auto producer = std::thread([&q, &push_timeouts, &timeout] () {
//     for (auto & result : push_timeouts)
//     {
//       result = q.push_wait_for(42, 1ns);
//     }
//   });

//   auto consumer = std::thread([&q, &pop_timeouts, &timeout] () {
//     auto elem = data_type { };
//     for (auto & result : pop_timeouts)
//     { result = q.pop_wait_for(elem, timeout); }
//   });

//   producer.join();
//   consumer.join();

//   CHECK(std::count(push_timeouts.begin(), push_timeouts.end(), false));
//   CHECK(std::count(pop_timeouts.begin(),  pop_timeouts.end(),  false));
// }



/****************************************************************************************\
  Benchmarks
\****************************************************************************************/



// TEST_CASE("Queue push* benchmarks", "[queue, push, benchmark]")
// {
//     auto q = queue<data_type, 100> { };
//     constexpr auto timeout = std::chrono::seconds(1);

//     BENCHMARK("queue::push")
//     {
//       q.push(42);
//     };
//     q.clear();

//     BENCHMARK("queue::push_wait")
//     {
//       q.push_wait(42);
//     };
//     q.clear();

//     BENCHMARK("queue::push_wait_for")
//     {
//       q.push_wait_for(42, timeout);
//     };
//     q.clear();
// }