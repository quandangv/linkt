#pragma once

#include "gtest/gtest.h"
#include "gtest/gtest-test-part.h"
#include "lini/languages.hpp"

#include <iostream>
#include <cmath>
#include <time.h>

using namespace ::testing;
using namespace lini;

using std::cerr, std::cout, std::vector, std::endl, std::string;

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

void check_key(const node::wrapper& w, string path, string expected, bool exception) {
  auto last_count = get_test_part_count();
  try {
    auto result = w.get_child(path);
    EXPECT_TRUE(result) << "Can't retrieve key";
    if (last_count == get_test_part_count())
      EXPECT_EQ(*result, expected) << "Unexpected value";
  } catch (const std::exception& e) {
    EXPECT_TRUE(exception) << "Unexpected exception thrown: " << e.what();
  }
  if (last_count != get_test_part_count())
    cerr << "Key: " << path << endl << endl;
}

void triple_node_test(node::base_p node, std::function<void(node::base_p)> tester, int repeat = 10000) {
  auto fail_count = get_test_part_count();
  auto time = get_time_milli();
  for (int i = 0; i < repeat; i++) {
    tester(node);
    if (fail_count != get_test_part_count())
      GTEST_SKIP() << "First test failed. Skipping clone test";
  }
  auto normal_time = get_time_milli() - time;

  node = clone(node);
  time = get_time_milli();
  for (int i = 0; i < repeat; i++) {
    tester(node);
    if (fail_count != get_test_part_count())
      GTEST_SKIP() << "Clone test failed. Skipping optimize test";
  }
  auto clone_time = get_time_milli() - time;

  node = clone(node, node::clone_mode::optimize | node::clone_mode::no_dependency);
  time = get_time_milli();
  for (int i = 0; i < repeat; i++) {
    tester(node);
    if (fail_count != get_test_part_count())
      GTEST_SKIP() << "Optimize test failed.";
  }
  auto optimize_time = get_time_milli() - time;
  cout << "Test time: normal " << normal_time << ", clone " << clone_time << ", optimize " << optimize_time << endl;
}
