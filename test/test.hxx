#pragma once

#include "gtest/gtest.h"
#include "gtest/gtest-test-part.h"
#include <linked_nodes/languages.hpp>

#include <iostream>
#include <cmath>
#include <time.h>

using namespace ::testing;

using std::cerr, std::cout, std::vector, std::endl, std::string;

constexpr int base_repeat = 300;
constexpr int print_time = false;

const TestResult& get_test_result() {
  auto error = std::runtime_error("Null pointer! Are we in a test?");
  return *(((UnitTest::GetInstance() ?: throw error)->current_test_info() ?: throw error)->result() ?: throw error);
}

// Failed ASSERT/EXPECT would cause the value returned to increase
// This can be used to track new failures
int get_test_part_count() {
  return get_test_result().total_part_count();
}

double get_time_milli() {
  timespec time;
  clock_gettime(CLOCK_MONOTONIC_RAW, &time);
  return round(time.tv_sec * 1000.0 + time.tv_nsec / 1000000.0);
}

void check_key(const node::wrapper& w, string path, string expected, bool exception, bool fixed = true) {
  auto last_count = get_test_part_count();
  try {
    auto result = w.get_child_ptr(path);
    EXPECT_TRUE(result) << "Can't retrieve key";
    if (result) {
      EXPECT_EQ(result->get(), expected) << "Unexpected value";
      EXPECT_EQ(result->is_fixed(), fixed);
    }
  } catch (const std::exception& e) {
    EXPECT_TRUE(exception) << "Unexpected exception thrown: " << e.what();
  }
  if (last_count != get_test_part_count())
    cerr << "Key: " << path << endl << endl;
}

void triple_node_test(node::base_s node, std::function<void(node::base_s, node::errorlist&)> tester, int repeat = base_repeat) {
  if (repeat <= 0)
    repeat = 1;
  auto fail_count = get_test_part_count();
  node::clone_context context;
  auto test = [&] {
    for (int i = 0; i < repeat; i++) {
      EXPECT_NO_THROW(tester(node, context.errors));
      if (fail_count != get_test_part_count())
        return true;
    }
    return false;
  };

  auto time = get_time_milli();
  if (test())
    GTEST_SKIP() << "First test failed. Skipping clone test";
  auto normal_time = get_time_milli() - time;

  EXPECT_NO_THROW(node = node->clone(context));
  if (test())
    GTEST_SKIP() << "Clone test failed. Skipping optimize test";

  context.optimize = context.no_dependency = true;
  EXPECT_NO_THROW(node = node->clone(context));
  time = get_time_milli();
  if (test())
    GTEST_SKIP() << "Optimize test failed.";
  auto optimize_time = get_time_milli() - time;

  if (print_time)
    cout << "Test time: normal " << std::setw(3) << normal_time << ", optimized " << std::setw(3) << optimize_time << endl;
}
